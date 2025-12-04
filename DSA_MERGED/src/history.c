#include "history.h"

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

static char *string_duplicate(const char *src) {
    if (!src) return NULL;
    size_t n = strlen(src);
    char *copy = (char *)checked_malloc(n + 1);
    memcpy(copy, src, n + 1);
    return copy;
}

void history_init(SearchHistory *history, size_t max_entries) {
    (void)max_entries; /* unused, stack has fixed max size */
    if (!history) return;
    history->count = 0;
    for (size_t i = 0; i < HISTORY_MAX_ENTRIES; ++i) {
        history->entries[i] = NULL;
    }
}

void history_record(SearchHistory *history, const char *query) {
    if (!history || !query || query[0] == '\0') return;

    /* if full, drop the oldest 10 at the bottom and shift others down */
    if (history->count == HISTORY_MAX_ENTRIES) {
        /* free bottom 10 entries */
        for (size_t i = 0; i < 10; ++i) {
            free(history->entries[i]);
            history->entries[i] = NULL;
        }
        /* shift remaining entries down by 10 positions */
        for (size_t i = 10; i < HISTORY_MAX_ENTRIES; ++i) {
            history->entries[i - 10] = history->entries[i];
            history->entries[i] = NULL;
        }
        history->count -= 10;
    }

    history->entries[history->count++] = string_duplicate(query);
}

void history_print(const SearchHistory *history) {
    if (!history || history->count == 0) {
        printf("No search history yet.\n");
        return;
    }
    printf("\n--- Search History (most recent first) ---\n");
    size_t shown = 0;
    char buffer[64];
    while (shown < history->count) {
        size_t remaining = history->count - shown;
        size_t page = remaining > 10 ? 10 : remaining;
        for (size_t i = 0; i < page; ++i) {
            size_t idx = history->count - 1 - (shown + i);
            printf("%2zu) %s\n", shown + i + 1, history->entries[idx]);
        }
        shown += page;
        if (shown >= history->count) break;
        printf("Show next 10 entries? (y/n): ");
        if (!fgets(buffer, sizeof(buffer), stdin)) break;
        if (buffer[0] != 'y' && buffer[0] != 'Y') break;
    }
}

void history_pop_last_ten(SearchHistory *history) {
    if (!history || history->count == 0) return;
    size_t num = history->count < 10 ? history->count : 10;
    for (size_t i = 0; i < num; ++i) {
        size_t idx = history->count - 1 - i;
        free(history->entries[idx]);
        history->entries[idx] = NULL;
    }
    history->count -= num;
}

void history_clear(SearchHistory *history) {
    if (!history) return;
    for (size_t i = 0; i < history->count; ++i) {
        free(history->entries[i]);
        history->entries[i] = NULL;
    }
    history->count = 0;
}
