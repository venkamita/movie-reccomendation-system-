#include "movie.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MOVIE_INITIAL_CAPACITY 1024
#define CSV_MAX_LINE 8192
#define CSV_MAX_FIELDS 64

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

static char *string_duplicate_lower(const char *src) {
    if (!src) return NULL;
    size_t n = strlen(src);
    char *copy = (char *)checked_malloc(n + 1);
    for (size_t i = 0; i < n; ++i) {
        copy[i] = (char)tolower((unsigned char)src[i]);
    }
    copy[n] = '\0';
    return copy;
}

static void string_trim(char *s) {
    if (!s) return;
    size_t len = strlen(s);
    size_t start = 0;
    while (start < len && isspace((unsigned char)s[start])) start++;
    size_t end = len;
    while (end > start && isspace((unsigned char)s[end - 1])) end--;
    if (start > 0) memmove(s, s + start, end - start);
    s[end - start] = '\0';
}

static int parse_csv_line(const char *line, char ***out_fields) {
    size_t capacity = 16;
    size_t count = 0;
    char **fields = (char **)checked_malloc(capacity * sizeof(char *));

    const char *p = line;
    while (*p) {
        if (count == capacity) {
            capacity *= 2;
            fields = (char **)realloc(fields, capacity * sizeof(char *));
            if (!fields) {
                fprintf(stderr, "Error: Out of memory during CSV parsing\n");
                exit(EXIT_FAILURE);
            }
        }

        while (*p == ' ' || *p == '\t') p++;

        int in_quotes = 0;
        if (*p == '"') { in_quotes = 1; p++; }

        char buffer[CSV_MAX_LINE];
        size_t bi = 0;
        while (*p) {
            if (in_quotes) {
                if (*p == '"') {
                    if (*(p + 1) == '"') {
                        if (bi + 1 >= sizeof(buffer)) break;
                        buffer[bi++] = '"';
                        p += 2;
                    } else {
                        p++;
                        in_quotes = 0;
                        while (*p == ' ' || *p == '\t') p++;
                        if (*p == ',') { p++; }
                        break;
                    }
                } else {
                    if (bi + 1 >= sizeof(buffer)) break;
                    buffer[bi++] = *p++;
                }
            } else {
                if (*p == ',') { p++; break; }
                if (*p == '\r' || *p == '\n') { break; }
                if (bi + 1 >= sizeof(buffer)) break;
                buffer[bi++] = *p++;
            }
        }
        buffer[bi] = '\0';

        char *field = string_duplicate(buffer);
        string_trim(field);
        fields[count++] = field;

        if (*p == '\r') p++;
        if (*p == '\n') p++;
        if (!*p) break;
    }

    *out_fields = fields;
    return (int)count;
}

static void free_fields(char **fields, int count) {
    if (!fields) return;
    for (int i = 0; i < count; ++i) free(fields[i]);
    free(fields);
}

static void movie_init(Movie *movie) {
    memset(movie, 0, sizeof(*movie));
}

static void movie_free(Movie *movie) {
    if (!movie) return;
    free(movie->show_id);
    free(movie->type);
    free(movie->title);
    free(movie->title_lower);
    free(movie->director);
    free(movie->director_lower);
    free(movie->cast);
    free(movie->country);
    free(movie->date_added);
    free(movie->release_year);
    free(movie->rating);
    free(movie->duration);
    free(movie->listed_in);
    free(movie->description);
    if (movie->genres) {
        for (size_t i = 0; i < movie->genre_count; ++i) {
            free(movie->genres[i]);
        }
        free(movie->genres);
    }
    memset(movie, 0, sizeof(*movie));
}

static void movie_parse_genres(Movie *movie) {
    movie->genres = NULL;
    movie->genre_count = 0;
    if (!movie->listed_in || movie->listed_in[0] == '\0') return;

    size_t capacity = 4;
    movie->genres = (char **)checked_malloc(capacity * sizeof(char *));

    char *working = string_duplicate(movie->listed_in);
    char *token = strtok(working, ",");
    while (token) {
        while (*token == ' ') token++;
        char *end = token + strlen(token);
        while (end > token && isspace((unsigned char)*(end - 1))) *(--end) = '\0';

        if (*token) {
            if (movie->genre_count == capacity) {
                capacity *= 2;
                movie->genres = (char **)realloc(movie->genres, capacity * sizeof(char *));
                if (!movie->genres) {
                    fprintf(stderr, "Error: Out of memory while storing genres\n");
                    exit(EXIT_FAILURE);
                }
            }
            movie->genres[movie->genre_count++] = string_duplicate_lower(token);
        }
        token = strtok(NULL, ",");
    }

    free(working);
}

static int normalize_header_index(char **fields, int count, const char *needle) {
    for (int i = 0; i < count; ++i) {
        const char *candidate = fields[i];
        if (!candidate) continue;
        size_t n = strlen(candidate);
        char *normalized = (char *)checked_malloc(n + 1);
        size_t pos = 0;
        for (size_t j = 0; j < n; ++j) {
            char c = (char)tolower((unsigned char)candidate[j]);
            if (c == ' ' || c == '\t' || c == '_') continue;
            normalized[pos++] = c;
        }
        normalized[pos] = '\0';
        int match = (strcmp(normalized, needle) == 0);
        free(normalized);
        if (match) return i;
    }
    return -1;
}

void movie_db_init(MovieDatabase *db) {
    if (!db) return;
    db->count = 0;
    db->capacity = MOVIE_INITIAL_CAPACITY;
    db->movies = (Movie *)checked_malloc(db->capacity * sizeof(Movie));
    for (size_t i = 0; i < db->capacity; ++i) {
        movie_init(&db->movies[i]);
    }
}

static int movie_db_grow(MovieDatabase *db) {
    size_t new_capacity = db->capacity * 2;
    Movie *new_movies = (Movie *)realloc(db->movies, new_capacity * sizeof(Movie));
    if (!new_movies) return 0;
    for (size_t i = db->capacity; i < new_capacity; ++i) {
        movie_init(&new_movies[i]);
    }
    db->movies = new_movies;
    db->capacity = new_capacity;
    return 1;
}

static void movie_assign_field(Movie *movie, char **fields, int count, int idx, char **out_storage) {
    (void)movie;
    const char *value = (idx >= 0 && idx < count && fields[idx]) ? fields[idx] : "";
    *out_storage = string_duplicate(value);
}

int movie_db_load_from_csv(MovieDatabase *db, const char *path, char **error_message) {
    if (error_message) *error_message = NULL;
    if (!db || !path) return 0;

    FILE *fp = fopen(path, "rb");
    if (!fp) {
        if (error_message) {
            size_t len = strlen(path) + 64;
            *error_message = (char *)checked_malloc(len);
            snprintf(*error_message, len, "Failed to open CSV file: %s", path);
        }
        return 0;
    }

    char line[CSV_MAX_LINE];
    if (!fgets(line, sizeof(line), fp)) {
        if (error_message) {
            *error_message = string_duplicate("CSV file appears to be empty or unreadable");
        }
        fclose(fp);
        return 0;
    }

    char **headers = NULL;
    int header_count = parse_csv_line(line, &headers);
    if (header_count <= 0) {
        if (error_message) {
            *error_message = string_duplicate("Failed to parse CSV header row");
        }
        fclose(fp);
        free_fields(headers, header_count);
        return 0;
    }

    int idx_show_id = normalize_header_index(headers, header_count, "showid");
    int idx_type = normalize_header_index(headers, header_count, "type");
    int idx_title = normalize_header_index(headers, header_count, "title");
    int idx_director = normalize_header_index(headers, header_count, "director");
    int idx_cast = normalize_header_index(headers, header_count, "cast");
    int idx_country = normalize_header_index(headers, header_count, "country");
    int idx_date_added = normalize_header_index(headers, header_count, "dateadded");
    int idx_release_year = normalize_header_index(headers, header_count, "releaseyear");
    int idx_rating = normalize_header_index(headers, header_count, "rating");
    int idx_duration = normalize_header_index(headers, header_count, "duration");
    int idx_listed_in = normalize_header_index(headers, header_count, "listedin");
    int idx_description = normalize_header_index(headers, header_count, "description");

    if (idx_title < 0) {
        if (error_message) {
            *error_message = string_duplicate("The CSV file does not contain a 'title' column.");
        }
        fclose(fp);
        free_fields(headers, header_count);
        return 0;
    }

    size_t loaded = 0;
    while (fgets(line, sizeof(line), fp)) {
        char **fields = NULL;
        int field_count = parse_csv_line(line, &fields);
        if (field_count <= 0) {
            free_fields(fields, field_count);
            continue;
        }

        if (db->count == db->capacity) {
            if (!movie_db_grow(db)) {
                if (error_message) {
                    *error_message = string_duplicate("Out of memory while expanding movie database.");
                }
                free_fields(fields, field_count);
                break;
            }
        }

        Movie *movie = &db->movies[db->count];
        movie_init(movie);

        movie_assign_field(movie, fields, field_count, idx_show_id, &movie->show_id);
        movie_assign_field(movie, fields, field_count, idx_type, &movie->type);
        movie_assign_field(movie, fields, field_count, idx_title, &movie->title);
        movie_assign_field(movie, fields, field_count, idx_director, &movie->director);
        movie_assign_field(movie, fields, field_count, idx_cast, &movie->cast);
        movie_assign_field(movie, fields, field_count, idx_country, &movie->country);
        movie_assign_field(movie, fields, field_count, idx_date_added, &movie->date_added);
        movie_assign_field(movie, fields, field_count, idx_release_year, &movie->release_year);
        movie_assign_field(movie, fields, field_count, idx_rating, &movie->rating);
        movie_assign_field(movie, fields, field_count, idx_duration, &movie->duration);
        movie_assign_field(movie, fields, field_count, idx_listed_in, &movie->listed_in);
        movie_assign_field(movie, fields, field_count, idx_description, &movie->description);

        movie->title_lower = string_duplicate_lower(movie->title);
        movie->director_lower = string_duplicate_lower(movie->director);
        movie->release_year_num = (movie->release_year && movie->release_year[0]) ? atoi(movie->release_year) : 0;

        movie_parse_genres(movie);

        db->count++;
        loaded++;
        free_fields(fields, field_count);
    }

    free_fields(headers, header_count);
    fclose(fp);

    if (loaded == 0 && error_message && !*error_message) {
        *error_message = string_duplicate("No movie records were loaded from the CSV file.");
    }

    return loaded > 0;
}

void movie_db_free(MovieDatabase *db) {
    if (!db) return;
    for (size_t i = 0; i < db->count; ++i) {
        movie_free(&db->movies[i]);
    }
    free(db->movies);
    db->movies = NULL;
    db->count = 0;
    db->capacity = 0;
}

