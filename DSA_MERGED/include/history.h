#ifndef HISTORY_H
#define HISTORY_H

#include <stddef.h>

#define HISTORY_MAX_ENTRIES 100

typedef struct {
	char *entries[HISTORY_MAX_ENTRIES];
	size_t count; // number of entries currently stored (stack size)
} SearchHistory;

void history_init(SearchHistory *history, size_t max_entries);
void history_record(SearchHistory *history, const char *query);
void history_print(const SearchHistory *history);
void history_clear(SearchHistory *history);
void history_pop_last_ten(SearchHistory *history);

#endif /* HISTORY_H */
