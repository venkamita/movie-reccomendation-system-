#ifndef SPLAY_H
#define SPLAY_H

#include <stddef.h>

typedef struct SplayNode {
    int key_score;                 /* recommendation score used for ordering */
    size_t movie_index;            /* reference to movie in MovieDatabase */
    struct SplayNode *left;
    struct SplayNode *right;
} SplayNode;

typedef struct {
    SplayNode *root;
    size_t size;
} SplayTree;

void splay_init(SplayTree *tree);
void splay_free(SplayTree *tree);

/* Insert (score, movie_index) and splay the inserted key to root. */
void splay_insert(SplayTree *tree, int score, size_t movie_index);

/* Bring the closest node for score to root (if exact key not found). */
void splay_access(SplayTree *tree, int score);

/* Query helpers for UI */
SplayNode *splay_root(const SplayTree *tree);

#endif /* SPLAY_H */

