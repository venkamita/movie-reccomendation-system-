#ifndef WATCHLIST_H
#define WATCHLIST_H

#include <stddef.h> //i am using this for size_t

#include "movie.h" 

#define MAX_WATCHLISTS 100
#define MAX_WATCHLIST_NAME_LEN 100

typedef struct WatchlistNode {
	size_t movie_index; // index into MovieDatabase->movies
	struct WatchlistNode *next;
} WatchlistNode;

typedef struct {
	char name[MAX_WATCHLIST_NAME_LEN + 1];
	WatchlistNode *head; // linked list of movies
	size_t count;  // number of movies in this watchlist
} Watchlist;

typedef struct {
	Watchlist lists[MAX_WATCHLISTS];
	size_t count; // number of watchlists currently in use
} WatchlistManager;

void watchlist_manager_init(WatchlistManager *manager);
void watchlist_manager_free(WatchlistManager *manager);

int watchlist_create(WatchlistManager *manager, const char *name);
int watchlist_delete(WatchlistManager *manager, size_t index);
int watchlist_rename(WatchlistManager *manager, size_t index, const char *new_name);

int watchlist_add_movie(WatchlistManager *manager, size_t index, size_t movie_index);
int watchlist_remove_movie(WatchlistManager *manager, size_t index, size_t position);

void watchlist_print_summary(const WatchlistManager *manager);
void watchlist_print_detail(const WatchlistManager *manager, const MovieDatabase *db, size_t index);

#endif /* WATCHLIST_H */

