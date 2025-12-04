#include "search.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void *checked_malloc(size_t size) {
    void *ptr = malloc(size);
    if (!ptr) {
        fprintf(stderr, "Error: Out of memory allocating %zu bytes\n", size);
        exit(EXIT_FAILURE);
    }
    return ptr;
}

static char *string_duplicate(const char *src) {
    if (!src) return NULL;
    size_t n = strlen(src);
    char *copy = (char *)checked_malloc(n + 1);
    memcpy(copy, src, n + 1);
    return copy;
}

static size_t next_power_of_two(size_t value) {
    size_t v = 1;
    while (v < value) v <<= 1;
    return v;
}

void title_index_init(TitleIndex *index) {
    if (!index) return;
    index->entries = NULL;
    index->capacity = 0;
    index->size = 0;
}

static void title_index_entry_free(TitleIndexEntry *entry) {
    if (!entry || !entry->occupied) return;
    free(entry->key_lower);
    free(entry->indices);
    entry->key_lower = NULL;
    entry->indices = NULL;
    entry->count = 0;
    entry->capacity = 0;
    entry->occupied = 0;
}

void title_index_free(TitleIndex *index) {
    if (!index) return;
    if (index->entries) {
        for (size_t i = 0; i < index->capacity; ++i) {
            title_index_entry_free(&index->entries[i]);
        }
        free(index->entries);
    }
    index->entries = NULL;
    index->capacity = 0;
    index->size = 0;
}

static int title_index_entry_append(TitleIndexEntry *entry, size_t movie_index) {
    if (entry->count == entry->capacity) {
        size_t new_capacity = entry->capacity == 0 ? 4 : entry->capacity * 2;
        size_t *new_indices = (size_t *)realloc(entry->indices, new_capacity * sizeof(size_t));
        if (!new_indices) return 0;
        entry->indices = new_indices;
        entry->capacity = new_capacity;
    }
    entry->indices[entry->count++] = movie_index;
    return 1;
}

static int title_index_insert(TitleIndex *index, const char *key_lower, size_t movie_index) {
    if (index->size * 100u / index->capacity >= 80u) {
        fprintf(stderr, "Warning: Title index load factor exceeded 80%%. Consider rebuilding with higher capacity.\n");
    }

    size_t hash = 5381u;
    for (const unsigned char *p = (const unsigned char *)key_lower; *p; ++p) {
        hash = ((hash << 5) + hash) + (size_t)(*p);
    }

    size_t idx = hash & (index->capacity - 1);
    size_t start = idx;
    while (1) {
        TitleIndexEntry *entry = &index->entries[idx];
        if (!entry->occupied) {
            entry->occupied = 1;
            entry->key_lower = string_duplicate(key_lower);
            entry->indices = NULL;
            entry->count = 0;
            entry->capacity = 0;
            index->size++;
        }
        if (strcmp(entry->key_lower, key_lower) == 0) {
            return title_index_entry_append(entry, movie_index);
        }
        idx = (idx + 1) & (index->capacity - 1);
        if (idx == start) {
            return 0;
        }
    }
}

int title_index_build(TitleIndex *index, const MovieDatabase *db) {
    if (!index || !db) return 0;
    title_index_free(index);
    size_t desired = db->count == 0 ? 16 : db->count * 2;
    size_t capacity = next_power_of_two(desired);
    if (capacity < 16) capacity = 16;

    index->entries = (TitleIndexEntry *)calloc(capacity, sizeof(TitleIndexEntry));
    if (!index->entries) {
        fprintf(stderr, "Error: Out of memory while building title index\n");
        return 0;
    }
    index->capacity = capacity;
    index->size = 0;

    for (size_t i = 0; i < db->count; ++i) {
        const Movie *movie = &db->movies[i];
        if (!movie->title_lower || movie->title_lower[0] == '\0') continue;
        if (!title_index_insert(index, movie->title_lower, i)) {
            fprintf(stderr, "Warning: Failed to insert movie title into index: %s\n", movie->title);
        }
    }
    return 1;
}

static int title_index_find_entry(const TitleIndex *index, const char *key_lower, TitleIndexEntry **out_entry) {
    if (!index || !index->entries || index->capacity == 0) return 0;

    size_t hash = 5381u;
    for (const unsigned char *p = (const unsigned char *)key_lower; *p; ++p) {
        hash = ((hash << 5) + hash) + (size_t)(*p);
    }

    size_t idx = hash & (index->capacity - 1);
    size_t start = idx;
    while (1) {
        TitleIndexEntry *entry = &index->entries[idx];
        if (!entry->occupied) {
            return 0;
        }
        if (strcmp(entry->key_lower, key_lower) == 0) {
            if (out_entry) *out_entry = entry;
            return 1;
        }
        idx = (idx + 1) & (index->capacity - 1);
        if (idx == start) return 0;
    }
}

static int allocate_result_copy(const TitleIndexEntry *entry, size_t **out_indices, size_t *out_count) {
    if (!entry || entry->count == 0) return 0;
    size_t *copy = (size_t *)checked_malloc(entry->count * sizeof(size_t));
    memcpy(copy, entry->indices, entry->count * sizeof(size_t));
    *out_indices = copy;
    *out_count = entry->count;
    return 1;
}

int title_index_lookup(const TitleIndex *index, const char *title_lower, size_t **out_indices, size_t *out_count) {
    if (out_indices) *out_indices = NULL;
    if (out_count) *out_count = 0;
    if (!index || !title_lower || !out_indices || !out_count) return 0;

    TitleIndexEntry *entry = NULL;
    if (!title_index_find_entry(index, title_lower, &entry)) {
        return 0;
    }
    return allocate_result_copy(entry, out_indices, out_count);
}

int title_index_partial_search(const TitleIndex *index, const char *needle_lower, size_t **out_indices, size_t *out_count) {
    if (out_indices) *out_indices = NULL;
    if (out_count) *out_count = 0;
    if (!index || !needle_lower || !out_indices || !out_count) return 0;

    size_t capacity = 16;
    size_t count = 0;
    size_t *results = NULL;

    for (size_t i = 0; i < index->capacity; ++i) {
        const TitleIndexEntry *entry = &index->entries[i];
        if (!entry->occupied) continue;
        if (strstr(entry->key_lower, needle_lower)) {
            for (size_t j = 0; j < entry->count; ++j) {
                size_t movie_index = entry->indices[j];
                int duplicate = 0;
                for (size_t k = 0; k < count; ++k) {
                    if (results[k] == movie_index) {
                        duplicate = 1;
                        break;
                    }
                }
                if (duplicate) continue;
                if (count == capacity) {
                    capacity *= 2;
                    size_t *grown = (size_t *)realloc(results, capacity * sizeof(size_t));
                    if (!grown) {
                        free(results);
                        return 0;
                    }
                    results = grown;
                }
                if (!results) {
                    results = (size_t *)checked_malloc(capacity * sizeof(size_t));
                }
                results[count++] = movie_index;
            }
        }
    }

    if (count == 0) {
        free(results);
        return 0;
    }

    *out_indices = results;
    *out_count = count;
    return 1;
}

static int append_index(size_t **buffer, size_t *count, size_t *capacity, size_t value) {
    if (*count == *capacity) {
        size_t new_capacity = (*capacity == 0) ? 16 : (*capacity * 2);
        size_t *grown = (size_t *)realloc(*buffer, new_capacity * sizeof(size_t));
        if (!grown) return 0;
        *buffer = grown;
        *capacity = new_capacity;
    }
    (*buffer)[(*count)++] = value;
    return 1;
}

int search_by_director(const MovieDatabase *db, const char *director_lower, size_t **out_indices, size_t *out_count) {
    if (out_indices) *out_indices = NULL;
    if (out_count) *out_count = 0;
    if (!db || !director_lower || !out_indices || !out_count) return 0;

    size_t *results = NULL;
    size_t count = 0;
    size_t capacity = 0;

    for (size_t i = 0; i < db->count; ++i) {
        const Movie *movie = &db->movies[i];
        if (!movie->director_lower) continue;
        if (strcmp(movie->director_lower, director_lower) == 0) {
            if (!append_index(&results, &count, &capacity, i)) {
                free(results);
                return 0;
            }
        }
    }

    if (count == 0) {
        free(results);
        return 0;
    }

    *out_indices = results;
    *out_count = count;
    return 1;
}

int search_by_director_partial(const MovieDatabase *db, const char *director_substr_lower, size_t **out_indices, size_t *out_count) {
    if (out_indices) *out_indices = NULL;
    if (out_count) *out_count = 0;
    if (!db || !director_substr_lower || !out_indices || !out_count) return 0;

    size_t *results = NULL;
    size_t count = 0;
    size_t capacity = 0;

    for (size_t i = 0; i < db->count; ++i) {
        const Movie *movie = &db->movies[i];
        if (!movie->director_lower) continue;
        if (strstr(movie->director_lower, director_substr_lower) != NULL) {
            if (!append_index(&results, &count, &capacity, i)) {
                free(results);
                return 0;
            }
        }
    }

    if (count == 0) {
        free(results);
        return 0;
    }

    *out_indices = results;
    *out_count = count;
    return 1;
}
int search_by_genre(const MovieDatabase *db, const char *genre_lower, size_t **out_indices, size_t *out_count) {
    if (out_indices) *out_indices = NULL;
    if (out_count) *out_count = 0;
    if (!db || !genre_lower || !out_indices || !out_count) return 0;

    size_t *results = NULL;
    size_t count = 0;
    size_t capacity = 0;

    for (size_t i = 0; i < db->count; ++i) {
        const Movie *movie = &db->movies[i];
        for (size_t j = 0; j < movie->genre_count; ++j) {
            if (strcmp(movie->genres[j], genre_lower) == 0) {
                if (!append_index(&results, &count, &capacity, i)) {
                    free(results);
                    return 0;
                }
                break;
            }
        }
    }

    if (count == 0) {
        free(results);
        return 0;
    }

    *out_indices = results;
    *out_count = count;
    return 1;
}

int search_by_genre_partial(const MovieDatabase *db, const char *genre_substr_lower, size_t **out_indices, size_t *out_count) {
    if (out_indices) *out_indices = NULL;
    if (out_count) *out_count = 0;
    if (!db || !genre_substr_lower || !out_indices || !out_count) return 0;

    size_t *results = NULL;
    size_t count = 0;
    size_t capacity = 0;

    for (size_t i = 0; i < db->count; ++i) {
        const Movie *movie = &db->movies[i];
        for (size_t j = 0; j < movie->genre_count; ++j) {
            if (strstr(movie->genres[j], genre_substr_lower) != NULL) {
                if (!append_index(&results, &count, &capacity, i)) {
                    free(results);
                    return 0;
                }
                break;
            }
        }
    }

    if (count == 0) {
        free(results);
        return 0;
    }

    *out_indices = results;
    *out_count = count;
    return 1;
}
int search_by_release_year(const MovieDatabase *db, int year, size_t **out_indices, size_t *out_count) {
    if (out_indices) *out_indices = NULL;
    if (out_count) *out_count = 0;
    if (!db || year <= 0 || !out_indices || !out_count) return 0;

    size_t *results = NULL;
    size_t count = 0;
    size_t capacity = 0;

    for (size_t i = 0; i < db->count; ++i) {
        const Movie *movie = &db->movies[i];
        if (movie->release_year_num == year) {
            if (!append_index(&results, &count, &capacity, i)) {
                free(results);
                return 0;
            }
        }
    }

    if (count == 0) {
        free(results);
        return 0;
    }

    *out_indices = results;
    *out_count = count;
    return 1;
}

