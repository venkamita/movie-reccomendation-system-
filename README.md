# 🎬 Movie Search & Recommendation System

A simple command-line movie search and recommendation system inspired by platforms like IMDb.  
Using a CSV dataset as the backend, the system provides movie search, history tracking, watchlist management, and personalized recommendations.

---

## 📌 Overview

This project emulates an online movie database in a simplified, offline, command-line format.  
Users can search for movies, view search history, maintain a watchlist, and receive recommendations based on what they have searched earlier.

The program demonstrates the use of multiple data structures to efficiently organize and retrieve movie data.

---

## ✨ Features

### 🔍 Search System
- Supports **exact match** and **partial match** movie searches.
- Fetches results from the CSV dataset.
- Built using efficient data structures for faster lookups.

### 🕘 Search History
- Stores all searches performed during runtime.
- Lets the user revisit previously viewed movies.
- Implemented using a **stack** and **linked lists**.

### ⭐ Watchlist
- Allows users to save movies they like.
- Easily accessible and organized.
- Implemented using **linked lists** and arrays.

### 🎯 Recommendation System
- Generates recommendations based on previous searches.
- Uses patterns in search history to suggest similar movies.
- Built using a **splay tree** and **hash map** for dynamic ranking.

---

## 🧱 Data Structures Used

- **Splay Tree**  
  For fast access to frequently searched movies.

- **Hash Map**  
  For efficient keyword and title indexing.

- **Linked Lists**  
  For watchlist and history management.

- **Stacks**  
  For tracking history order.

- **Arrays**  
  For storing dataset records and search results.

---

## 📂 Dataset

The project uses:

[netflix dataset november 2019](https://www.kaggle.com/datasets/ajitsharadmane/netflix-titles-nov-2019)

This acts as the base movie database.

---

## ▶️ How to Run the Program

1. Download or clone the repository.
2. Open the project folder in any code editor **or** navigate to it in a terminal.

### Compile the Program

```bash
gcc -std=c11 -Wall -Wextra -Wpedantic -Wshadow -Wconversion \
-Iinclude \
src/main.c src/movie.c src/search.c src/history.c src/watchlist.c \
src/recommendation.c src/splay.c src/reco_tree.c \
-o movie_explorer
```
### Run the Program
```bash
./movie_explorer data/netflix_titles_nov_2019.csv
```
## Credits:
[Sharat Doddihal](https://github.com/venkamita)
