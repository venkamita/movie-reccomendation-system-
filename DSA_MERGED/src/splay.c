#include "splay.h"

#include <stdio.h>
#include <stdlib.h>

static SplayNode *new_node(int score, size_t movie_index) {
    SplayNode *n = (SplayNode *)malloc(sizeof(SplayNode));
    if (!n) { perror("malloc"); exit(1); }
    n->key_score = score;
    n->movie_index = movie_index;
    n->left = n->right = NULL;
    return n;
}

static SplayNode *rotate_right(SplayNode *x) {
    SplayNode *y = x->left;
    x->left = y->right;
    y->right = x;
    return y;
}

static SplayNode *rotate_left(SplayNode *x) {
    SplayNode *y = x->right;
    x->right = y->left;
    y->left = x;
    return y;
}

static SplayNode *splay(SplayNode *root, int key) {
    if (!root) return NULL;
    SplayNode header = {0, 0, NULL, NULL};
    SplayNode *LeftTreeMax = &header;
    SplayNode *RightTreeMin = &header;

    while (1) {
        if (key < root->key_score) {
            if (!root->left) break;
            if (key < root->left->key_score) {
                root = rotate_right(root);
                if (!root->left) break;
            }
            RightTreeMin->left = root;
            RightTreeMin = root;
            root = root->left;
            RightTreeMin->left = NULL;
        } else if (key > root->key_score) {
            if (!root->right) break;
            if (key > root->right->key_score) {
                root = rotate_left(root);
                if (!root->right) break;
            }
            LeftTreeMax->right = root;
            LeftTreeMax = root;
            root = root->right;
            LeftTreeMax->right = NULL;
        } else {
            break;
        }
    }

    LeftTreeMax->right = root->left;
    RightTreeMin->left = root->right;
    root->left = header.right;
    root->right = header.left;
    return root;
}

void splay_init(SplayTree *tree) {
    tree->root = NULL;
    tree->size = 0;
}

static void free_nodes(SplayNode *n) {
    if (!n) return;
    free_nodes(n->left);
    free_nodes(n->right);
    free(n);
}

void splay_free(SplayTree *tree) {
    if (!tree) return;
    free_nodes(tree->root);
    tree->root = NULL;
    tree->size = 0;
}

void splay_access(SplayTree *tree, int score) {
    if (!tree->root) return;
    tree->root = splay(tree->root, score);
}

void splay_insert(SplayTree *tree, int score, size_t movie_index) {
    if (!tree->root) {
        tree->root = new_node(score, movie_index);
        tree->size = 1;
        return;
    }
    tree->root = splay(tree->root, score);
    if (score == tree->root->key_score) {
        /* same-key insert: prefer to keep first; adjust score slightly to keep BST property */
        score += 1;
    }
    SplayNode *n = new_node(score, movie_index);
    if (score < tree->root->key_score) {
        n->right = tree->root;
        n->left = tree->root->left;
        tree->root->left = NULL;
    } else {
        n->left = tree->root;
        n->right = tree->root->right;
        tree->root->right = NULL;
    }
    tree->root = n;
    tree->size++;
}

SplayNode *splay_root(const SplayTree *tree) {
    return tree ? tree->root : NULL;
}


