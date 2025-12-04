#ifndef MOVIE_H
#define MOVIE_H

#include <stddef.h>

typedef struct {
    char *show_id;
    char *type;
    char *title;
    char *title_lower;
    char *director;
    char *director_lower;
    char *cast;
    char *country;
    char *date_added;
    char *release_year;
    int release_year_num;
    char *rating;
    char *duration;
    char *listed_in;
    char *description;
    char **genres;
    size_t genre_count;
} Movie;

typedef struct {
    Movie *movies;
    size_t count;
    size_t capacity;
} MovieDatabase;

void movie_db_init(MovieDatabase *db);
int movie_db_load_from_csv(MovieDatabase *db, const char *path, char **error_message);
void movie_db_free(MovieDatabase *db);

#endif /* MOVIE_H */

