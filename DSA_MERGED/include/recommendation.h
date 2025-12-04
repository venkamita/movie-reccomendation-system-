#ifndef RECOMMENDATION_H
#define RECOMMENDATION_H

#include <stddef.h>

#include "movie.h"

typedef struct {
    size_t movie_index;
    int score;
    int genre_overlap;
    int year_diff;
    int director_match;
} Recommendation;

int recommendation_generate(const MovieDatabase *db, size_t source_index, Recommendation **out_list, size_t *out_count);
void recommendation_print(const MovieDatabase *db, const Recommendation *list, size_t count, size_t limit);

#endif /* RECOMMENDATION_H */

