#include "recommendation.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int genre_overlap_count(const Movie *a, const Movie *b) {
    int count = 0;
    for (size_t i = 0; i < a->genre_count; ++i) {
        for (size_t j = 0; j < b->genre_count; ++j) {
            if (strcmp(a->genres[i], b->genres[j]) == 0) {
                count++;
                break;
            }
        }
    }
    return count;
}

static int recommendation_compare(const void *lhs, const void *rhs) {
    const Recommendation *a = (const Recommendation *)lhs;
    const Recommendation *b = (const Recommendation *)rhs;
    if (a->score != b->score) return (b->score - a->score);
    if (a->genre_overlap != b->genre_overlap) return (b->genre_overlap - a->genre_overlap);
    if (a->director_match != b->director_match) return (b->director_match - a->director_match);
    return (a->year_diff - b->year_diff);
}

int recommendation_generate(const MovieDatabase *db, size_t source_index, Recommendation **out_list, size_t *out_count) {
    if (out_list) *out_list = NULL;
    if (out_count) *out_count = 0;
    if (!db || !out_list || !out_count || source_index >= db->count) return 0;
    if (db->count <= 1) return 0;

    const Movie *source = &db->movies[source_index];
    Recommendation *list = (Recommendation *)malloc((db->count - 1) * sizeof(Recommendation));
    if (!list) {
        fprintf(stderr, "Error: Unable to allocate recommendation buffer\n");
        return 0;
    }

    size_t count = 0;
    for (size_t i = 0; i < db->count; ++i) {
        if (i == source_index) continue;
        const Movie *candidate = &db->movies[i];
        int overlap = genre_overlap_count(source, candidate);
        int director_match = 0;
        if (source->director_lower && candidate->director_lower &&
            source->director_lower[0] && candidate->director_lower[0] &&
            strcmp(source->director_lower, candidate->director_lower) == 0) {
            director_match = 1;
        }
        int year_diff;
        if (source->release_year_num > 0 && candidate->release_year_num > 0) {
            year_diff = abs(source->release_year_num - candidate->release_year_num);
        } else {
            year_diff = 1000;
        }
        int score = overlap * 100 + (director_match ? 50 : 0) - year_diff;

        list[count].movie_index = i;
        list[count].score = score;
        list[count].genre_overlap = overlap;
        list[count].year_diff = year_diff;
        list[count].director_match = director_match;
        count++;
    }

    qsort(list, count, sizeof(Recommendation), recommendation_compare);

    *out_list = list;
    *out_count = count;
    return 1;
}

void recommendation_print(const MovieDatabase *db, const Recommendation *list, size_t count, size_t limit) {
    if (!db || !list || count == 0) {
        printf("No recommendations available.\n");
        return;
    }
    if (limit == 0 || limit > count) limit = count;
    printf("\nTop %zu recommendation(s):\n", limit);
    for (size_t i = 0; i < limit; ++i) {
        size_t idx = list[i].movie_index;
        if (idx >= db->count) continue;
        const Movie *movie = &db->movies[idx];
        printf("%2zu) %s (%s)  [score=%d, genres=%d%s]\n",
               i + 1,
               movie->title ? movie->title : "(no title)",
               movie->release_year ? movie->release_year : "n/a",
               list[i].score,
               list[i].genre_overlap,
               list[i].director_match ? ", same director" : "");
    }
}

