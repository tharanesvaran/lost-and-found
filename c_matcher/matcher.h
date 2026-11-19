/**
 * ============================================================================
 * College Lost & Found System
 * Module: Intelligent Similarity Scoring & Match Suggestion Engine
 * Language: C (C99 / C11 Standard)
 * Author: Tharanesvaran (tharanesvaran07@gmail.com)
 *
 * Capabilities:
 *   1. Lexical Synonym Canonicalization (e.g., specs <-> spectacles -> glasses)
 *   2. 2D Euclidean Spatial Distance & Radius Decay Engine (Campus Map Geometry)
 *   3. Multi-factor Similarity Scoring & Threshold Evaluation (>= 75%)
 * ============================================================================
 */

#ifndef MATCHER_H
#define MATCHER_H

#ifdef __cplusplus
extern "C" {
#endif

/* Maximum possible similarity score */
#define MATCH_MAX_SCORE 12

/* Minimum percentage required to suggest an item to a student */
#define MATCH_MIN_PERCENT 75

/* Minimum score required to suggest an item (equivalent to 75%) */
#define MATCH_MIN_SCORE 9

/* Maximum time proximity window: 7 days in minutes (7 * 24 * 60) */
#define MATCH_WEEK_MINUTES 10080

/* Scoring weight values */
#define WEIGHT_CATEGORY 3
#define WEIGHT_LOCATION_EXACT 2
#define WEIGHT_LOCATION_NEARBY 1
#define WEIGHT_DATE_PROXIMITY 1
#define WEIGHT_SHARED_WORD 2
#define MAX_SHARED_WORDS_SCORED 3

/* Spatial radius squared thresholds on campus 2D map (540x440 canvas) */
#define SPATIAL_EXACT_RADIUS_SQ   2025   /* <= 45 px radius: Exact Spot (+2 pts) */
#define SPATIAL_NEARBY_RADIUS_SQ  16900  /* <= 130 px radius: Nearby Zone (+1 pt) */

/* Bitmask flags explaining why an item matched */
#define WHY_SAME_CATEGORY    (1 << 0) /* 1: Same category (+3) */
#define WHY_SAME_LOCATION    (1 << 1) /* 2: Exact same spot / zone (+2) */
#define WHY_WITHIN_WEEK      (1 << 2) /* 4: Found within 7 days (+1) */
#define WHY_KEYWORD_MATCH    (1 << 3) /* 8: Has matching keywords (+2 per word, max 6) */
#define WHY_NEARBY_LOCATION  (1 << 4) /* 16: Proximity match within campus radius (+1) */

/* Structure representing an item (either reported lost or logged as found) */
typedef struct {
    char id[64];
    char category[32];
    char location[64];
    char description[256];
    long timestamp_minutes; /* Minutes since Unix epoch */
    int x;                  /* Campus 2D map X coordinate (0 if not pinned) */
    int y;                  /* Campus 2D map Y coordinate (0 if not pinned) */
} Item;

/* Structure representing similarity evaluation outcome */
typedef struct {
    int score;         /* 0 to 12 */
    int percent;       /* 0 to 100% */
    int why_flags;     /* Bitmask of reasons (WHY_*) */
    int shared_words;  /* Count of common keywords */
    int is_match;      /* 1 if percent >= 75 (score >= 9), 0 otherwise */
    int distance_sq;   /* Squared Euclidean distance on campus map (or -1 if unpinned) */
} MatchResult;

/* Structure for campus synonym mapping / lexical canonicalization */
typedef struct {
    const char* canonical;        /* Canonical root term (e.g. "glasses") */
    const char* aliases[8];       /* Common synonyms (e.g. "specs", "spectacles") */
    int alias_count;
} SynonymGroup;

/* Core mathematical scoring functions (exported to WebAssembly / usable in C) */
int score_pair_c(int same_cat, int same_loc, int within_week, int shared_words, int* out_why_flags);
int score_pair_spatial_c(int same_cat, int loc_score, int within_week, int shared_words, int* out_why_flags);
int percent_of_c(int score);
int is_match_c(int score);

/* Spatial Distance & Radius Decay Engine functions */
int calculate_spatial_distance_sq_c(int x1, int y1, int x2, int y2);
int calculate_spatial_score_c(int x1, int y1, int x2, int y2, int same_zone);

/* High-level item evaluation function */
MatchResult evaluate_match_c(const Item* lost, const Item* found);

/* Lexical canonicalization & synonym mapping */
const char* get_canonical_term_c(const char* word);

/* String tokenization helpers */
void str_to_lower_c(char* dest, const char* src, int max_len);
int extract_keywords_c(const char* text, char tokens[][32], int max_tokens);
int count_shared_keywords_c(char words1[][32], int count1, char words2[][32], int count2);

#ifdef __cplusplus
}
#endif

#endif /* MATCHER_H */
