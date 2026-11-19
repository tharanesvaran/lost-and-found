/**
 * ============================================================================
 * College Lost & Found System
 * Module: Intelligent Similarity Scoring & Match Suggestion Engine
 * Language: C (C99 / C11 Standard)
 * Author: Tharanesvaran (tharanesvaran07@gmail.com)
 *
 * Core Modules:
 *   1. Lexical Synonym Canonicalization (specs <-> spectacles -> glasses)
 *   2. 2D Euclidean Spatial Distance & Radius Decay Engine (Map Geometry)
 *   3. Multi-factor Similarity Scoring & Threshold Evaluation (>= 75%)
 *   4. Fuzzy Typo-Tolerant Search (Wagner-Fischer Levenshtein Distance)
 *   5. Complex 2D Polygon Ray-Casting (Jordan Curve Theorem for Campus Zones)
 *   6. Anti-Theft Verification Question & Answer Similarity Normalizer
 *   7. Holding Period Expiration & Disposition Finite State Machine (FSM)
 *   8. Okapi BM25 / Keyword Relevance Search Ranking
 *   9. Campus Loss Analytics & Statistical Recovery Aggregator
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
#define WHY_SAME_LOCATION    (1 << 1) /* 2: Exact spot / zone (+2) */
#define WHY_WITHIN_WEEK      (1 << 2) /* 4: Found within 7 days (+1) */
#define WHY_KEYWORD_MATCH    (1 << 3) /* 8: Has matching keywords (+2 per word, max 6) */
#define WHY_NEARBY_LOCATION  (1 << 4) /* 16: Proximity match within campus radius (+1) */

/* 2D Geometric Point */
typedef struct {
    int x;
    int y;
} Point;

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

/* Enumeration of legal holding disposition states */
typedef enum {
    DISP_ACTIVE_UNCLAIMED  = 0, /* Under standard retention window (<= 45 days) */
    DISP_GRACE_PERIOD      = 1, /* Final 7-day grace period */
    DISP_ELIGIBLE_AUCTION  = 2, /* Valuables (phones, laptops) eligible for college auction */
    DISP_ELIGIBLE_DONATION = 3, /* General items (books, bottles, clothes) for student charity */
    DISP_EXPIRED_ARCHIVE   = 4  /* Sensitive documents/records archived or destroyed */
} DispositionState;

/* Summary report structure for Campus Loss Analytics */
typedef struct {
    int total_lost;
    int total_found;
    int total_returned;
    int recovery_rate_pct;         /* (returned * 100) / found */
    int avg_resolution_time_hrs;   /* Average turnaround hours from loss to handover */
    char highest_risk_zone[64];    /* Zone with the highest loss frequency */
} CampusAnalyticsSummary;

/* ============================================================================
 * FUNCTION DECLARATIONS
 * ============================================================================ */

/* --- 1. Core Similarity Match Engine --- */
int score_pair_c(int same_cat, int same_loc, int within_week, int shared_words, int* out_why_flags);
int score_pair_spatial_c(int same_cat, int loc_score, int within_week, int shared_words, int* out_why_flags);
int percent_of_c(int score);
int is_match_c(int score);
MatchResult evaluate_match_c(const Item* lost, const Item* found);

/* --- 2. Spatial Distance & Radius Decay Engine --- */
int calculate_spatial_distance_sq_c(int x1, int y1, int x2, int y2);
int calculate_spatial_score_c(int x1, int y1, int x2, int y2, int same_zone);

/* --- 3. Lexical Synonym Canonicalization --- */
const char* get_canonical_term_c(const char* word);
void str_to_lower_c(char* dest, const char* src, int max_len);
int extract_keywords_c(const char* text, char tokens[][32], int max_tokens);
int count_shared_keywords_c(char words1[][32], int count1, char words2[][32], int count2);

/* --- 4. Fuzzy Search & Typo-Tolerance Engine (Feature 1) --- */
int levenshtein_distance_c(const char* s1, const char* s2);
int fuzzy_match_score_c(const char* pattern, const char* text);

/* --- 5. Complex 2D Polygon Ray-Casting (Feature 2) --- */
int point_in_polygon_c(Point pt, const Point* vertices, int num_vertices);
const char* detect_campus_zone_polygon_c(int px, int py);

/* --- 6. Verification Answer Matching & Phonetic Tolerance (Feature 3) --- */
int verify_answer_similarity_c(const char* student_ans, const char* recorded_ans);
int is_answer_accepted_c(const char* student_ans, const char* recorded_ans, int threshold_percent);

/* --- 7. Holding Period Expiration & Disposition State Machine (Feature 4) --- */
DispositionState evaluate_disposition_state_c(const char* category, long found_at_minutes, long current_minutes, int base_hold_days, int* out_days_left);
const char* disposition_state_name_c(DispositionState state);

/* --- 8. Okapi BM25 / Keyword Relevance Search Ranking (Feature 5) --- */
int compute_search_relevance_c(const char* query, const char* item_title, const char* item_desc);

/* --- 9. Campus Loss Analytics & Statistical Aggregation Engine (Feature 6) --- */
CampusAnalyticsSummary aggregate_campus_analytics_c(const Item* lost_list, int lost_count, const Item* found_list, int found_count, const int* resolution_hours, int resolved_count);

#ifdef __cplusplus
}
#endif

#endif /* MATCHER_H */
