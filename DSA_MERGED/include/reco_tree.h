#ifndef RECO_TREE_H
#define RECO_TREE_H

#include <stddef.h>

#include "movie.h"
#include "recommendation.h"
#include "splay.h"

typedef struct {
    SplayTree tree;
    int has_source;
    size_t source_index;
} RecommendationTree;

void reco_tree_init(RecommendationTree *rt);
void reco_tree_free(RecommendationTree *rt);

/* Rebuild/augment the splay tree using recommendations from source_index. */
int reco_tree_update_from_source(RecommendationTree *rt, const MovieDatabase *db, size_t source_index, size_t topn);

/* Show root and immediate children for quick UI peek. */
void reco_tree_print_root_and_children(const RecommendationTree *rt, const MovieDatabase *db);

/* Collect movie indices in descending score order into out_indices (length returned). */
size_t reco_tree_collect_descending(const RecommendationTree *rt, size_t *out_indices, size_t max_out);

#endif /* RECO_TREE_H */

