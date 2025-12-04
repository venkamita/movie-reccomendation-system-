#include "watchlist.h"

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

static void watchlist_init(Watchlist *list) {
    if (!list) return;
    list->name[0] = '\0';
    list->head = NULL;
    list->count = 0;
}

static void watchlist_clear_nodes(Watchlist *list) {
    if (!list) return;
    WatchlistNode *cur = list->head;
    while (cur) {
        WatchlistNode *next = cur->next;
        free(cur);
        cur = next;
    }
    list->head = NULL;
    list->count = 0;
}

static void watchlist_free(Watchlist *list) {
    if (!list) return;
    watchlist_clear_nodes(list);
    watchlist_init(list);
}

void watchlist_manager_init(WatchlistManager *manager) {
    if (!manager) return;
    manager->count = 0;
    for (size_t i = 0; i < MAX_WATCHLISTS; ++i) {
        watchlist_init(&manager->lists[i]);
    }
}

int watchlist_create(WatchlistManager *manager, const char *name) {
    if (!manager || !name || name[0] == '\0') return 0;
    if (manager->count >= MAX_WATCHLISTS) {
        fprintf(stderr, "Error: Maximum number of watchlists (%d) reached\n", MAX_WATCHLISTS);
        return 0;
    }
    Watchlist *list = &manager->lists[manager->count];
    watchlist_init(list);
    strncpy(list->name, name, MAX_WATCHLIST_NAME_LEN);
    list->name[MAX_WATCHLIST_NAME_LEN] = '\0';
    manager->count++;
    return 1;
}

int watchlist_delete(WatchlistManager *manager, size_t index) {
    if (!manager || index >= manager->count) return 0;
    watchlist_free(&manager->lists[index]);
    if (index != manager->count - 1) {
        memmove(&manager->lists[index], &manager->lists[index + 1], (manager->count - index - 1) * sizeof(Watchlist));
    }
    manager->count--;
    return 1;
}

int watchlist_rename(WatchlistManager *manager, size_t index, const char *new_name) {
    if (!manager || index >= manager->count || !new_name || new_name[0] == '\0') return 0;
    Watchlist *list = &manager->lists[index];
    strncpy(list->name, new_name, MAX_WATCHLIST_NAME_LEN);
    list->name[MAX_WATCHLIST_NAME_LEN] = '\0';
    return 1;
}

int watchlist_add_movie(WatchlistManager *manager, size_t index, size_t movie_index) {
    if (!manager || index >= manager->count) return 0;
    Watchlist *list = &manager->lists[index];
    WatchlistNode *node = (WatchlistNode *)checked_malloc(sizeof(WatchlistNode));
    node->movie_index = movie_index;
    node->next = NULL;
    if (!list->head) {
        list->head = node;
    } else {
        WatchlistNode *cur = list->head;
        while (cur->next) {
            cur = cur->next;
        }
        cur->next = node;
    }
    list->count++;
    return 1;
}

int watchlist_remove_movie(WatchlistManager *manager, size_t index, size_t position) {
    if (!manager || index >= manager->count) return 0;
    Watchlist *list = &manager->lists[index];
    if (position == 0 || position > list->count || !list->head) return 0;
    size_t target = position - 1; // 0-based
    WatchlistNode *prev = NULL;
    WatchlistNode *cur = list->head;
    while (cur && target > 0) {
        prev = cur;
        cur = cur->next;
        --target;
    }
    if (!cur) return 0;
    if (prev) {
        prev->next = cur->next;
    } else {
        list->head = cur->next;
    }
    free(cur);
    list->count--;
    return 1;
}

void watchlist_print_summary(const WatchlistManager *manager) {
    if (!manager || manager->count == 0) {
        printf("(no watchlists created yet)\n");
        return;
    }
    for (size_t i = 0; i < manager->count; ++i) {
        const Watchlist *list = &manager->lists[i];
        const char *name = (list->name[0] != '\0') ? list->name : "(untitled)";
        printf("%2zu) %s [%zu movies]\n", i + 1, name, list->count);
    }
}

void watchlist_print_detail(const WatchlistManager *manager, const MovieDatabase *db, size_t index) {
    if (!manager || index >= manager->count) {
        printf("Invalid watchlist selection.\n");
        return;
    }
    const Watchlist *list = &manager->lists[index];
    const char *name = (list->name[0] != '\0') ? list->name : "(untitled)";
    printf("\nWatchlist: %s\n", name);
    if (!list->head || list->count == 0) {
        printf("  (no movies added yet)\n");
        return;
    }
    WatchlistNode *cur = list->head;
    size_t i = 0;
    while (cur) {
        size_t movie_index = cur->movie_index;
        if (movie_index >= db->count) {
            printf("  %2zu) [invalid movie reference]\n", i + 1);
        } else {
            const Movie *movie = &db->movies[movie_index];
            printf("  %2zu) %s (%s)\n", i + 1,
                movie->title ? movie->title : "(no title)",
                movie->release_year ? movie->release_year : "n/a");
        }
        cur = cur->next;
        ++i;
    }
}

void watchlist_manager_free(WatchlistManager *manager) {
    if (!manager) return;
    for (size_t i = 0; i < manager->count; ++i) {
        watchlist_free(&manager->lists[i]);
    }
    manager->count = 0;
}
