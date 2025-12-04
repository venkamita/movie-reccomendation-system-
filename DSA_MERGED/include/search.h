#ifndef SEARCH_H
#define SEARCH_H

#include <stddef.h>

#include "movie.h"

typedef struct {
    char *key_lower;
    size_t *indices;
    size_t count;
    size_t capacity;
    int occupied;
} TitleIndexEntry;

typedef struct {
    TitleIndexEntry *entries;
    size_t capacity;
    size_t size;
} TitleIndex;

void title_index_init(TitleIndex *index);
int title_index_build(TitleIndex *index, const MovieDatabase *db);
void title_index_free(TitleIndex *index);

int title_index_lookup(const TitleIndex *index, const char *title_lower, size_t **out_indices, size_t *out_count);
int title_index_partial_search(const TitleIndex *index, const char *needle_lower, size_t **out_indices, size_t *out_count);

int search_by_director(const MovieDatabase *db, const char *director_lower, size_t **out_indices, size_t *out_count);
int search_by_director_partial(const MovieDatabase *db, const char *director_substr_lower, size_t **out_indices, size_t *out_count);
int search_by_genre(const MovieDatabase *db, const char *genre_lower, size_t **out_indices, size_t *out_count);
int search_by_genre_partial(const MovieDatabase *db, const char *genre_substr_lower, size_t **out_indices, size_t *out_count);
int search_by_release_year(const MovieDatabase *db, int year, size_t **out_indices, size_t *out_count);

#endif /* SEARCH_H */

