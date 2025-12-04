#include "reco_tree.h"

#include <stdio.h>
#include <stdlib.h>

void reco_tree_init(RecommendationTree *rt) {
    splay_init(&rt->tree);
    rt->has_source = 0;
    rt->source_index = 0;
}

void reco_tree_free(RecommendationTree *rt) {
    splay_free(&rt->tree);
    rt->has_source = 0;
    rt->source_index = 0;
}

int reco_tree_update_from_source(RecommendationTree *rt, const MovieDatabase *db, size_t source_index, size_t topn) {
    if (!rt || !db || source_index >= db->count) return 0;
    Recommendation *list = NULL;
    size_t count = 0;
    if (!recommendation_generate(db, source_index, &list, &count)) {
        free(list);
        return 0;
    }
    if (topn == 0 || topn > count) topn = count;
    /* Insert topn recommendations into splay tree; splay on each insert so most-recent high-score moves near root */
    for (size_t i = 0; i < topn; ++i) {
        splay_insert(&rt->tree, list[i].score, list[i].movie_index);
    }
    rt->has_source = 1;
    rt->source_index = source_index;
    free(list);
    return 1;
}

void reco_tree_print_root_and_children(const RecommendationTree *rt, const MovieDatabase *db) {
    if (!rt) { printf("Recommendation tree not initialized.\n"); return; }
    const SplayNode *root = splay_root(&rt->tree);
    if (!root) {
        printf("No recommendations yet.\n");
        return;
    }
    printf("\nRecommendation Tree (root and immediate children):\n");
    if (root->movie_index < db->count) {
        const Movie *m = &db->movies[root->movie_index];
        printf("Root: %s (%s)\n", m->title ? m->title : "(no title)", m->release_year ? m->release_year : "n/a");
    } else {
        printf("Root: [invalid movie index]\n");
    }
    if (root->left) {
        if (root->left->movie_index < db->count) {
            const Movie *ml = &db->movies[root->left->movie_index];
            printf("  Left : %s (%s)\n", ml->title ? ml->title : "(no title)", ml->release_year ? ml->release_year : "n/a");
        } else {
            printf("  Left : [invalid]\n");
        }
    }
    if (root->right) {
        if (root->right->movie_index < db->count) {
            const Movie *mr = &db->movies[root->right->movie_index];
            printf("  Right: %s (%s)\n", mr->title ? mr->title : "(no title)", mr->release_year ? mr->release_year : "n/a");
        } else {
            printf("  Right: [invalid]\n");
        }
    }
}

static size_t collect_desc(const SplayNode *n, size_t *out, size_t max_out, size_t written) {
    if (!n || written >= max_out) return written;
    written = collect_desc(n->right, out, max_out, written);
    if (written < max_out) {
        out[written++] = n->movie_index;
    }
    if (written < max_out) {
        written = collect_desc(n->left, out, max_out, written);
    }
    return written;
}

size_t reco_tree_collect_descending(const RecommendationTree *rt, size_t *out_indices, size_t max_out) {
    if (!rt || !out_indices || max_out == 0) return 0;
    return collect_desc(rt->tree.root, out_indices, max_out, 0);
}


