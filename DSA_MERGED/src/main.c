#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "history.h"
#include "movie.h"
#include "recommendation.h"
#include "reco_tree.h"
#include "search.h"
#include "watchlist.h"

#define INPUT_BUFFER 512
#define DEFAULT_DATASET "data/netflix_titles_nov_2019.csv"

static void trim_newline(char *s) {
    if (!s) return;
    size_t len = strlen(s);
    while (len > 0 && (s[len - 1] == '\n' || s[len - 1] == '\r')) {
        s[--len] = '\0';
    }
}

static void to_lower_inplace(char *s) {
    if (!s) return;
    for (; *s; ++s) {
        *s = (char)tolower((unsigned char)*s);
    }
}

static void press_enter_to_continue(void) {
    printf("\nPress Enter to continue...");
    char buffer[INPUT_BUFFER];
    fgets(buffer, sizeof(buffer), stdin);
}

static void print_movie_details(const Movie *movie) {
    if (!movie) return;
    printf("\nTitle       : %s\n", movie->title ? movie->title : "(unknown)");
    printf("Type        : %s\n", movie->type ? movie->type : "n/a");
    printf("Director    : %s\n", movie->director && movie->director[0] ? movie->director : "n/a");
    printf("Cast        : %s\n", movie->cast && movie->cast[0] ? movie->cast : "n/a");
    printf("Country     : %s\n", movie->country && movie->country[0] ? movie->country : "n/a");
    printf("Date Added  : %s\n", movie->date_added && movie->date_added[0] ? movie->date_added : "n/a");
    printf("Release Year: %s\n", movie->release_year && movie->release_year[0] ? movie->release_year : "n/a");
    printf("Rating      : %s\n", movie->rating && movie->rating[0] ? movie->rating : "n/a");
    printf("Duration    : %s\n", movie->duration && movie->duration[0] ? movie->duration : "n/a");
    printf("Genres      : %s\n", movie->listed_in && movie->listed_in[0] ? movie->listed_in : "n/a");
    printf("Description : %s\n", movie->description && movie->description[0] ? movie->description : "n/a");
}

static void prompt_add_to_watchlist(const MovieDatabase *db,
                                    WatchlistManager *watchlists,
                                    size_t movie_index) {
    if (!db || !watchlists || movie_index >= db->count) return;
    char buffer[INPUT_BUFFER];
    printf("\nAdd this movie to a watchlist?\n");
    printf(" 1) Yes\n");
    printf(" 2) No\n");
    printf("Choose: ");
    if (!fgets(buffer, sizeof(buffer), stdin)) return;
    trim_newline(buffer);
    char *endptr = NULL;
    long yesno = strtol(buffer, &endptr, 10);
    if (endptr == buffer || (yesno != 1 && yesno != 2)) return;
    if (yesno == 2) return;

    while (1) {
        printf("\nWatchlists:\n");
        watchlist_print_summary(watchlists);
        printf("\nWhat would you like to do?\n");
        printf(" 1) Add to existing watchlist\n");
        printf(" 2) Create new watchlist\n");
        printf(" 3) Cancel\n");
        printf("Choose: ");
        if (!fgets(buffer, sizeof(buffer), stdin)) return;
        trim_newline(buffer);
        char *np = NULL;
        long choice = strtol(buffer, &np, 10);
        if (np == buffer || choice < 1 || choice > 3) { printf("Invalid choice.\n"); continue; }
        if (choice == 3) return;
        if (choice == 2) {
            printf("Enter name for new watchlist: ");
            if (!fgets(buffer, sizeof(buffer), stdin)) return;
            trim_newline(buffer);
            if (buffer[0] == '\0') { printf("Name cannot be empty.\n"); continue; }
            if (!watchlist_create(watchlists, buffer)) { printf("Failed to create watchlist.\n"); continue; }
            size_t new_index = watchlists->count - 1;
            if (watchlist_add_movie(watchlists, new_index, movie_index)) {
                printf("Added to watchlist '%s'.\n", watchlists->lists[new_index].name);
            } else {
                printf("Failed to add movie to watchlist.\n");
            }
            return;
        }
        /* choice == 1: add to existing */
        if (watchlists->count == 0) { printf("No existing watchlists. Please create one first.\n"); continue; }
        printf("Enter watchlist number: ");
        if (!fgets(buffer, sizeof(buffer), stdin)) return;
        trim_newline(buffer);
        np = NULL;
        long num = strtol(buffer, &np, 10);
        if (np == buffer || num <= 0 || (size_t)num > watchlists->count) { printf("Invalid watchlist number.\n"); continue; }
        size_t watchlist_index = (size_t)(num - 1);
        if (watchlist_add_movie(watchlists, watchlist_index, movie_index)) {
            printf("Added to watchlist '%s'.\n", watchlists->lists[watchlist_index].name);
        } else {
            printf("Failed to add movie to watchlist.\n");
        }
        return;
    }
}

static int g_has_last_viewed = 0;
static size_t g_last_viewed_index = 0;

static void show_search_results(const MovieDatabase *db,
                                WatchlistManager *watchlists,
                                SearchHistory *history,
                                const size_t *indices,
                                size_t count) {
    if (!db || !indices || count == 0) {
        printf("No matches found.\n");
        return;
    }
    size_t display = count > 25 ? 25 : count;
    printf("\nFound %zu match(es). Showing first %zu:\n", count, display);
    for (size_t i = 0; i < display; ++i) {
        size_t idx = indices[i];
        if (idx >= db->count) continue;
        const Movie *movie = &db->movies[idx];
        printf("%2zu) %s (%s)\n", i + 1,
               movie->title ? movie->title : "(no title)",
               movie->release_year ? movie->release_year : "n/a");
    }
    char buffer[INPUT_BUFFER];
    while (1) {
        printf("\nEnter a result number to view details, or press Enter to return: ");
        if (!fgets(buffer, sizeof(buffer), stdin)) return;
        trim_newline(buffer);
        if (buffer[0] == '\0') return;
        char *endptr = NULL;
        long choice = strtol(buffer, &endptr, 10);
        if (endptr == buffer || choice <= 0 || (size_t)choice > display) {
            printf("Invalid selection.\n");
            continue;
        }
        size_t result_index = indices[choice - 1];
        if (result_index >= db->count) {
            printf("Internal error: movie out of range.\n");
            return;
        }
        const Movie *movie = &db->movies[result_index];
        print_movie_details(movie);
        /* record viewed movie title into history */
        if (history && movie->title && movie->title[0]) {
            history_record(history, movie->title);
        }
        /* remember last viewed to drive recommendations later (from menu) */
        g_has_last_viewed = 1;
        g_last_viewed_index = result_index;
        prompt_add_to_watchlist(db, watchlists, result_index);
        return;
    }
}

static void search_menu(const MovieDatabase *db,
                        TitleIndex *index,
                        SearchHistory *history,
                        WatchlistManager *watchlists,
                        RecommendationTree *reco) {
    (void)reco; /* recommendations shown only via menu, not here */
    if (!db || !index || !history || !watchlists) return;
    char buffer[INPUT_BUFFER];
    while (1) {
        printf("\n--- Search Menu ---\n");
        printf(" 1) Exact title search\n");
        printf(" 2) Partial title search\n");
        printf(" 3) Search by director\n");
        printf(" 4) Search by genre (examples: drama, comedy, thriller, horror, action, romance, documentary, kids, anime)\n");
        printf(" 5) Search by release year\n");
        printf(" 6) Back to main menu\n");
        printf("Choose: ");
        if (!fgets(buffer, sizeof(buffer), stdin)) return;
        trim_newline(buffer);
        if (buffer[0] == '6' || buffer[0] == '\0') return;

        size_t *indices = NULL;
        size_t count = 0;
        char query[INPUT_BUFFER];

        switch (buffer[0]) {
            case '1':
                printf("Enter movie title: ");
                if (!fgets(query, sizeof(query), stdin)) break;
                trim_newline(query);
                if (query[0] == '\0') break;
                history_record(history, query);
                {
                    char lowered[INPUT_BUFFER];
                    strncpy(lowered, query, sizeof(lowered));
                    lowered[sizeof(lowered) - 1] = '\0';
                    to_lower_inplace(lowered);
                    if (title_index_lookup(index, lowered, &indices, &count)) {
                        show_search_results(db, watchlists, history, indices, count);
                        free(indices);
                    } else {
                        printf("No exact matches for '%s'.\n", query);
                    }
                }
                break;
            case '2':
                printf("Enter search term: ");
                if (!fgets(query, sizeof(query), stdin)) break;
                trim_newline(query);
                if (query[0] == '\0') break;
                history_record(history, query);
                {
                    char lowered[INPUT_BUFFER];
                    strncpy(lowered, query, sizeof(lowered));
                    lowered[sizeof(lowered) - 1] = '\0';
                    to_lower_inplace(lowered);
                    if (title_index_partial_search(index, lowered, &indices, &count)) {
                        show_search_results(db, watchlists, history, indices, count);
                        free(indices);
                    } else {
                        printf("No partial matches for '%s'.\n", query);
                    }
                }
                break;
            case '3':
                printf("Enter director name: ");
                if (!fgets(query, sizeof(query), stdin)) break;
                trim_newline(query);
                if (query[0] == '\0') break;
                history_record(history, query);
                {
                    char lowered[INPUT_BUFFER];
                    strncpy(lowered, query, sizeof(lowered));
                    lowered[sizeof(lowered) - 1] = '\0';
                    to_lower_inplace(lowered);
                    if (search_by_director_partial(db, lowered, &indices, &count)) {
                        show_search_results(db, watchlists, history, indices, count);
                        free(indices);
                    } else {
                        printf("No matches for director '%s'.\n", query);
                    }
                }
                break;
            case '4':
                printf("Enter genre (partial allowed, case-insensitive): ");
                if (!fgets(query, sizeof(query), stdin)) break;
                trim_newline(query);
                if (query[0] == '\0') break;
                history_record(history, query);
                {
                    char lowered[INPUT_BUFFER];
                    strncpy(lowered, query, sizeof(lowered));
                    lowered[sizeof(lowered) - 1] = '\0';
                    to_lower_inplace(lowered);
                    if (search_by_genre_partial(db, lowered, &indices, &count)) {
                        show_search_results(db, watchlists, history, indices, count);
                        free(indices);
                    } else {
                        printf("No matches for genre '%s'.\n", query);
                    }
                }
                break;
            case '5':
                printf("Enter release year: ");
                if (!fgets(query, sizeof(query), stdin)) break;
                trim_newline(query);
                if (query[0] == '\0') break;
                history_record(history, query);
                {
                    char *endptr = NULL;
                    long year = strtol(query, &endptr, 10);
                    if (endptr == query || year <= 0) {
                        printf("Invalid year.\n");
                        break;
                    }
                    if (search_by_release_year(db, (int)year, &indices, &count)) {
                        show_search_results(db, watchlists, history, indices, count);
                        free(indices);
                    } else {
                        printf("No matches for year %ld.\n", year);
                    }
                }
                break;
            default:
                printf("Invalid option.\n");
                break;
        }
    }
}

static void watchlist_menu(WatchlistManager *watchlists, const MovieDatabase *db) {
    if (!watchlists || !db) return;
    char buffer[INPUT_BUFFER];
    while (1) {
        printf("\n--- Watchlists ---\n");
        printf(" 1) Show summary\n");
        printf(" 2) View watchlist details\n");
        printf(" 3) Create watchlist\n");
        printf(" 4) Rename watchlist\n");
        printf(" 5) Delete watchlist\n");
        printf(" 6) Back\n");
        printf("Choose: ");
        if (!fgets(buffer, sizeof(buffer), stdin)) return;
        trim_newline(buffer);
        if (buffer[0] == '6' || buffer[0] == '\0') return;

        switch (buffer[0]) {
            case '1':
                printf("\nCurrent watchlists:\n");
                watchlist_print_summary(watchlists);
                press_enter_to_continue();
                break;
            case '2': {
                if (watchlists->count == 0) {
                    printf("No watchlists to show.\n");
                    break;
                }
                watchlist_print_summary(watchlists);
                printf("Select watchlist number: ");
                if (!fgets(buffer, sizeof(buffer), stdin)) break;
                trim_newline(buffer);
                char *endptr = NULL;
                long choice = strtol(buffer, &endptr, 10);
                if (endptr == buffer || choice <= 0 || (size_t)choice > watchlists->count) {
                    printf("Invalid selection.\n");
                    break;
                }
                size_t index = (size_t)(choice - 1);
                watchlist_print_detail(watchlists, db, index);
                if (index < watchlists->count && watchlists->lists[index].count > 0) {
                    printf("\nEnter a movie number to remove (or 0 to cancel): ");
                    if (!fgets(buffer, sizeof(buffer), stdin)) break;
                    trim_newline(buffer);
                    long remove_choice = strtol(buffer, &endptr, 10);
                    if (endptr != buffer && remove_choice > 0) {
                        if (watchlist_remove_movie(watchlists, index, (size_t)remove_choice)) {
                            printf("Removed movie %ld from watchlist.\n", remove_choice);
                        } else {
                            printf("Failed to remove movie.\n");
                        }
                    }
                }
                press_enter_to_continue();
                break;
            }
            case '3':
                printf("Enter new watchlist name: ");
                if (!fgets(buffer, sizeof(buffer), stdin)) break;
                trim_newline(buffer);
                if (buffer[0] == '\0') {
                    printf("Name cannot be empty.\n");
                    break;
                }
                if (watchlist_create(watchlists, buffer)) {
                    printf("Created watchlist '%s'.\n", buffer);
                } else {
                    printf("Failed to create watchlist.\n");
                }
                break;
            case '4':
                if (watchlists->count == 0) {
                    printf("No watchlists to rename.\n");
                    break;
                }
                watchlist_print_summary(watchlists);
                printf("Select watchlist number: ");
                if (!fgets(buffer, sizeof(buffer), stdin)) break;
                trim_newline(buffer);
                char *endptr2 = NULL;
                long rename_choice = strtol(buffer, &endptr2, 10);
                if (endptr2 == buffer || rename_choice <= 0 || (size_t)rename_choice > watchlists->count) {
                    printf("Invalid selection.\n");
                    break;
                }
                size_t rename_index = (size_t)(rename_choice - 1);
                printf("Enter new name: ");
                if (!fgets(buffer, sizeof(buffer), stdin)) break;
                trim_newline(buffer);
                if (buffer[0] == '\0') {
                    printf("Name cannot be empty.\n");
                    break;
                }
                if (watchlist_rename(watchlists, rename_index, buffer)) {
                    printf("Watchlist renamed.\n");
                } else {
                    printf("Failed to rename watchlist.\n");
                }
                break;
            case '5':
                if (watchlists->count == 0) {
                    printf("No watchlists to delete.\n");
                    break;
                }
                watchlist_print_summary(watchlists);
                printf("Select watchlist number to delete: ");
                if (!fgets(buffer, sizeof(buffer), stdin)) break;
                trim_newline(buffer);
                char *endptr3 = NULL;
                long delete_choice = strtol(buffer, &endptr3, 10);
                if (endptr3 == buffer || delete_choice <= 0 || (size_t)delete_choice > watchlists->count) {
                    printf("Invalid selection.\n");
                    break;
                }
                size_t delete_index = (size_t)(delete_choice - 1);
                printf("Are you sure you want to delete '%s'? (y/n): ",
                       watchlists->lists[delete_index].name ? watchlists->lists[delete_index].name : "(untitled)");
                if (!fgets(buffer, sizeof(buffer), stdin)) break;
                trim_newline(buffer);
                if (buffer[0] == 'y' || buffer[0] == 'Y') {
                    if (watchlist_delete(watchlists, delete_index)) {
                        printf("Watchlist deleted.\n");
                    } else {
                        printf("Failed to delete watchlist.\n");
                    }
                } else {
                    printf("Cancelled.\n");
                }
                break;
            default:
                printf("Invalid option.\n");
                break;
        }
    }
}

static void recommendation_menu(const MovieDatabase *db,
                                TitleIndex *index,
                                WatchlistManager *watchlists,
                                RecommendationTree *reco) {
    if (!db || !index || !reco) return;
    (void)watchlists; /* currently unused in this menu */
    char buffer[INPUT_BUFFER];
    printf("\n--- Recommendations ---\n");
    if (!splay_root(&reco->tree)) {
        if (g_has_last_viewed) {
            /* Build from last viewed automatically */
            if (!reco_tree_update_from_source(reco, db, g_last_viewed_index, 20)) {
                printf("No recommendations yet. View a movie from search first.\n");
                return;
            }
        } else {
            printf("No recommendations yet. View a movie from search first.\n");
            return;
        }
    }
    /* Page through results from the existing tree (no root/children labels) */
    size_t order[512];
    size_t total = reco_tree_collect_descending(reco, order, sizeof(order)/sizeof(order[0]));
    size_t shown = 0;
    while (shown < total) {
        size_t to_show = total - shown;
        if (to_show > 10) to_show = 10;
        printf("\nMore recommendations:\n");
        for (size_t i = 0; i < to_show; ++i) {
            size_t mi = order[shown + i];
            if (mi < db->count) {
                const Movie *rm = &db->movies[mi];
                printf("  %2zu) %s (%s)\n", shown + i + 1, rm->title ? rm->title : "(no title)", rm->release_year ? rm->release_year : "n/a");
            }
        }
        shown += to_show;
        if (shown >= total) break;
        printf("\nEnter number of additional recommendations to show (0 to back, max %zu): ", total - shown > 20 ? (size_t)20 : (total - shown));
        if (!fgets(buffer, sizeof(buffer), stdin)) break;
        trim_newline(buffer);
        char *ep = NULL;
        long more = strtol(buffer, &ep, 10);
        if (ep == buffer || more <= 0) break;
        if ((size_t)more > (total - shown)) more = (long)(total - shown);
        printf("\n");
        for (long i = 0; i < more; ++i) {
            size_t mi = order[shown + (size_t)i];
            if (mi < db->count) {
                const Movie *rm = &db->movies[mi];
                printf("  %2zu) %s (%s)\n", shown + (size_t)i + 1, rm->title ? rm->title : "(no title)", rm->release_year ? rm->release_year : "n/a");
            }
        }
        shown += (size_t)more;
    }
}

static int reload_dataset(MovieDatabase *db, TitleIndex *index, const char *path) {
    if (!db || !index || !path) return 0;
    movie_db_free(db);
    movie_db_init(db);
    char *error = NULL;
    if (!movie_db_load_from_csv(db, path, &error)) {
        if (error) {
            fprintf(stderr, "%s\n", error);
            free(error);
        } else {
            fprintf(stderr, "Failed to load dataset from %s\n", path);
        }
        return 0;
    }
    if (!title_index_build(index, db)) {
        fprintf(stderr, "Failed to build search index.\n");
        return 0;
    }
    return 1;
}

int main(int argc, char **argv) {
    const char *dataset_path = (argc >= 2) ? argv[1] : DEFAULT_DATASET;
    MovieDatabase db;
    movie_db_init(&db);
    TitleIndex title_index;
    title_index_init(&title_index);
    SearchHistory history;
    history_init(&history, 200);
    WatchlistManager watchlists;
    watchlist_manager_init(&watchlists);
    RecommendationTree reco;
    reco_tree_init(&reco);

    if (!reload_dataset(&db, &title_index, dataset_path)) {
        printf("Would you like to provide a different dataset path? (y/n): ");
        char buffer[INPUT_BUFFER];
        if (fgets(buffer, sizeof(buffer), stdin)) {
            trim_newline(buffer);
            if (buffer[0] == 'y' || buffer[0] == 'Y') {
                printf("Enter CSV file path: ");
                if (fgets(buffer, sizeof(buffer), stdin)) {
                    trim_newline(buffer);
                    if (!reload_dataset(&db, &title_index, buffer)) {
                        printf("Failed to load dataset. Exiting.\n");
                        goto cleanup;
                    }
                }
            } else {
                goto cleanup;
            }
        } else {
            goto cleanup;
        }
    }

    printf("Loaded %zu movie entries from %s\n", db.count, dataset_path);

    char input[INPUT_BUFFER];
    while (1) {

        printf("\n=== Movie Explorer ===\n");
        printf(" 1) Search movies\n");
        printf(" 2) View search history\n");
        printf(" 3) Manage watchlists\n");
        printf(" 4) Get recommendations\n");
        printf(" 5) Exit\n");
        printf("Choose: ");
        if (!fgets(input, sizeof(input), stdin)) break;
        trim_newline(input);
        if (input[0] == '5' || input[0] == '\0') {
            printf("Goodbye!\n");
            break;
        }

        switch (input[0]) {
            case '1':
                search_menu(&db, &title_index, &history, &watchlists, &reco);
                break;
            case '2':
                history_print(&history);
                press_enter_to_continue();
                break;
            case '3':
                watchlist_menu(&watchlists, &db);
                break;
            case '4':
                recommendation_menu(&db, &title_index, &watchlists, &reco);
                press_enter_to_continue();
                break;
            default:
                printf("Invalid choice. Please try again.\n");
                break;
        }
    }

cleanup:
    title_index_free(&title_index);
    watchlist_manager_free(&watchlists);
    history_clear(&history);
    reco_tree_free(&reco);
    movie_db_free(&db);
    return 0;
}
