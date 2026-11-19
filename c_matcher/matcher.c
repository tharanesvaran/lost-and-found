/**
 * ============================================================================
 * College Lost & Found System
 * Module: Intelligent Similarity Scoring & Match Suggestion Engine
 * Language: C (C99 / C11 Standard)
 * Author: Tharanesvaran (tharanesvaran07@gmail.com)
 *
 * This module computes the mathematical similarity between lost items reported
 * by students and verified found items cataloged by administrators.
 *
 * Scoring Formula:
 *   - Same Category:       +3 points
 *   - Spatial Proximity:   +2 points (Exact spot <= 45px or unpinned same zone)
 *                          +1 point  (Nearby zone radius decay <= 130px)
 *   - Found within 7 days: +1 point
 *   - Shared canonical:    +2 points per word (capped at 3 words = max 6 points)
 *   --------------------------------------------------------------------------
 *   Total Maximum Score:   12 points
 *   Percentage:            floor((score * 100 + 6) / 12)
 *   Match Threshold:       >= 75% (Score 9 or higher triggers student alerts)
 * ============================================================================
 */

#if defined(__has_include)
  #if __has_include("matcher.h")
    #include "matcher.h"
  #endif
#endif

#ifndef MATCHER_H
#define MATCHER_H

#define MATCH_MAX_SCORE 12
#define MATCH_MIN_PERCENT 75
#define MATCH_MIN_SCORE 9
#define MATCH_WEEK_MINUTES 10080

#define WEIGHT_CATEGORY 3
#define WEIGHT_LOCATION_EXACT 2
#define WEIGHT_LOCATION_NEARBY 1
#define WEIGHT_DATE_PROXIMITY 1
#define WEIGHT_SHARED_WORD 2
#define MAX_SHARED_WORDS_SCORED 3

#define SPATIAL_EXACT_RADIUS_SQ   2025   /* <= 45 px radius: Exact Spot (+2 pts) */
#define SPATIAL_NEARBY_RADIUS_SQ  16900  /* <= 130 px radius: Nearby Zone (+1 pt) */

#define WHY_SAME_CATEGORY    (1 << 0) /* 1: Same category (+3) */
#define WHY_SAME_LOCATION    (1 << 1) /* 2: Exact spot / zone (+2) */
#define WHY_WITHIN_WEEK      (1 << 2) /* 4: Found within 7 days (+1) */
#define WHY_KEYWORD_MATCH    (1 << 3) /* 8: Has matching keywords (+2 per word, max 6) */
#define WHY_NEARBY_LOCATION  (1 << 4) /* 16: Proximity match within campus radius (+1) */

typedef struct {
    char id[64];
    char category[32];
    char location[64];
    char description[256];
    long timestamp_minutes;
    int x;
    int y;
} Item;

typedef struct {
    int score;
    int percent;
    int why_flags;
    int shared_words;
    int is_match;
    int distance_sq;
} MatchResult;

typedef struct {
    const char* canonical;
    const char* aliases[8];
    int alias_count;
} SynonymGroup;

int score_pair_c(int same_cat, int same_loc, int within_week, int shared_words, int* out_why_flags);
int score_pair_spatial_c(int same_cat, int loc_score, int within_week, int shared_words, int* out_why_flags);
int percent_of_c(int score);
int is_match_c(int score);
int calculate_spatial_distance_sq_c(int x1, int y1, int x2, int y2);
int calculate_spatial_score_c(int x1, int y1, int x2, int y2, int same_zone);
MatchResult evaluate_match_c(const Item* lost, const Item* found);
const char* get_canonical_term_c(const char* word);

#endif /* MATCHER_H */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#if defined(_WIN32) && !defined(strcasecmp)
#define strcasecmp _stricmp
#endif

/* Campus Synonym Dictionary for Lexical Canonicalization */
static const SynonymGroup CAMPUS_SYNONYMS[] = {
    {"phone",      {"mobile", "cellphone", "smartphone", "handset", "iphone", "android"}, 6},
    {"glasses",    {"specs", "spectacles", "shades", "eyeglasses", "goggles"}, 5},
    {"earphones",  {"earbuds", "airpods", "headphones", "headset", "earpiece", "pods"}, 6},
    {"bag",        {"backpack", "rucksack", "knapsack", "satchel", "pouch"}, 5},
    {"drive",      {"pendrive", "flashdrive", "thumbdrive", "usbdrive"}, 4},
    {"idcard",     {"badge", "hallticket", "identitycard", "idbadge"}, 4},
    {"wallet",     {"purse", "billfold", "moneybag", "clutch"}, 4},
    {"bottle",     {"flask", "sipper", "waterbottle", "thermos"}, 4},
    {"calculator", {"calci", "scientificcalculator", "casio"}, 3},
    {"watch",      {"smartwatch", "timepiece", "wristwatch", "fastrack", "fitbit"}, 5},
    {"charger",    {"adapter", "cable", "powerbank", "cord"}, 4},
    {"umbrella",   {"parasol", "brolly", "rainshade"}, 3},
    {"keys",       {"keychain", "bikekey", "scootykey", "keyring"}, 4},
    {"laptop",     {"notebook", "macbook", "computer", "thinkpad"}, 4}
};

/**
 * Resolves any synonym / alias to its canonical campus root word in O(1) time.
 */
const char* get_canonical_term_c(const char* word) {
    if (!word || !word[0]) return "";
    int total_groups = (int)(sizeof(CAMPUS_SYNONYMS) / sizeof(CAMPUS_SYNONYMS[0]));
    for (int i = 0; i < total_groups; i++) {
        if (strcasecmp(word, CAMPUS_SYNONYMS[i].canonical) == 0) {
            return CAMPUS_SYNONYMS[i].canonical;
        }
        for (int j = 0; j < CAMPUS_SYNONYMS[i].alias_count; j++) {
            if (strcasecmp(word, CAMPUS_SYNONYMS[i].aliases[j]) == 0) {
                return CAMPUS_SYNONYMS[i].canonical;
            }
        }
    }
    return word;
}

/**
 * Computes squared Euclidean distance: d^2 = (x1 - x2)^2 + (y1 - y2)^2
 * Uses integer math to avoid floating-point square root overhead.
 */
int calculate_spatial_distance_sq_c(int x1, int y1, int x2, int y2) {
    int dx = x1 - x2;
    int dy = y1 - y2;
    return (dx * dx) + (dy * dy);
}

/**
 * Calculates spatial match score (0, 1, or 2 points) using campus map geometry:
 *   - If coordinates missing (<= 0): fallback to same_zone ? 2 : 0
 *   - Spot radius (dist <= 45px => d^2 <= 2025): 2 points
 *   - Nearby zone radius (dist <= 130px => d^2 <= 16900): 1 point (spatial decay)
 *   - Distant: same_zone ? 1 : 0
 */
int calculate_spatial_score_c(int x1, int y1, int x2, int y2, int same_zone) {
    if (x1 <= 0 || x2 <= 0) {
        return same_zone ? WEIGHT_LOCATION_EXACT : 0;
    }
    int d2 = calculate_spatial_distance_sq_c(x1, y1, x2, y2);
    if (d2 <= SPATIAL_EXACT_RADIUS_SQ) {
        return WEIGHT_LOCATION_EXACT;
    }
    if (d2 <= SPATIAL_NEARBY_RADIUS_SQ) {
        return WEIGHT_LOCATION_NEARBY;
    }
    return same_zone ? WEIGHT_LOCATION_NEARBY : 0;
}

/**
 * Calculates score (0-12) taking spatial location score (0, 1, or 2) directly.
 */
int score_pair_spatial_c(int same_cat, int loc_score, int within_week, int shared_words, int* out_why_flags) {
    int s = 0;
    int w = 0;

    if (same_cat) {
        s += WEIGHT_CATEGORY;
        w |= WHY_SAME_CATEGORY;
    }
    if (loc_score == 2) {
        s += WEIGHT_LOCATION_EXACT;
        w |= WHY_SAME_LOCATION;
    } else if (loc_score == 1) {
        s += WEIGHT_LOCATION_NEARBY;
        w |= WHY_NEARBY_LOCATION;
    }
    if (within_week) {
        s += WEIGHT_DATE_PROXIMITY;
        w |= WHY_WITHIN_WEEK;
    }

    int shared = shared_words > MAX_SHARED_WORDS_SCORED ? MAX_SHARED_WORDS_SCORED : shared_words;
    if (shared > 0) {
        w |= WHY_KEYWORD_MATCH;
    }
    s += shared * WEIGHT_SHARED_WORD;
    w |= (shared << 5); /* Shift past bit 4 */

    if (out_why_flags) {
        *out_why_flags = w;
    }
    return s;
}

/**
 * Backward compatibility wrapper for binary location flag.
 */
int score_pair_c(int same_cat, int same_loc, int within_week, int shared_words, int* out_why_flags) {
    return score_pair_spatial_c(same_cat, same_loc ? 2 : 0, within_week, shared_words, out_why_flags);
}

/**
 * Calculates percentage integer (0-100) with nearest-half rounding.
 */
int percent_of_c(int score) {
    if (score <= 0) return 0;
    if (score >= MATCH_MAX_SCORE) return 100;
    return (score * 100 + (MATCH_MAX_SCORE / 2)) / MATCH_MAX_SCORE;
}

/**
 * Checks whether the score satisfies the minimum 75% threshold (score >= 9).
 */
int is_match_c(int score) {
    return score >= MATCH_MIN_SCORE ? 1 : 0;
}

/**
 * Helper: Converts an ASCII string to lowercase.
 */
void str_to_lower_c(char* dest, const char* src, int max_len) {
    int i = 0;
    while (src[i] && i < max_len - 1) {
        dest[i] = (char)tolower((unsigned char)src[i]);
        i++;
    }
    dest[i] = '\0';
}

/**
 * Helper: Extracts distinct words and canonicalizes synonyms using the dictionary.
 */
int extract_keywords_c(const char* text, char tokens[][32], int max_tokens) {
    int token_count = 0;
    int len = (int)strlen(text);
    char buf[32];
    int buf_idx = 0;

    for (int i = 0; i <= len; i++) {
        char ch = text[i];
        if (isalpha((unsigned char)ch)) {
            if (buf_idx < 31) {
                buf[buf_idx++] = (char)tolower((unsigned char)ch);
            }
        } else {
            if (buf_idx >= 3) {
                buf[buf_idx] = '\0';
                const char* canon = get_canonical_term_c(buf);
                int exists = 0;
                for (int j = 0; j < token_count; j++) {
                    if (strcmp(tokens[j], canon) == 0) {
                        exists = 1;
                        break;
                    }
                }
                if (!exists && token_count < max_tokens) {
                    strncpy(tokens[token_count], canon, 31);
                    tokens[token_count][31] = '\0';
                    token_count++;
                }
            }
            buf_idx = 0;
        }
    }
    return token_count;
}

/**
 * Helper: Counts common unique keywords between two token lists.
 */
int count_shared_keywords_c(char words1[][32], int count1, char words2[][32], int count2) {
    int shared = 0;
    for (int i = 0; i < count1; i++) {
        for (int j = 0; j < count2; j++) {
            if (strcmp(words1[i], words2[j]) == 0) {
                shared++;
                break;
            }
        }
    }
    return shared;
}

/**
 * Evaluates similarity between a lost report and a found item with full spatial geometry.
 */
MatchResult evaluate_match_c(const Item* lost, const Item* found) {
    MatchResult res;
    memset(&res, 0, sizeof(res));

    /* 1. Category check */
    int same_cat = (strcasecmp(lost->category, found->category) == 0) ? 1 : 0;

    /* 2. Campus zone / spatial geometry check */
    int same_zone = (strcasecmp(lost->location, found->location) == 0) ? 1 : 0;
    int loc_score = calculate_spatial_score_c(lost->x, lost->y, found->x, found->y, same_zone);

    if (lost->x > 0 && found->x > 0) {
        res.distance_sq = calculate_spatial_distance_sq_c(lost->x, lost->y, found->x, found->y);
    } else {
        res.distance_sq = -1;
    }

    /* 3. Time proximity check (within 7 days = 10,080 minutes) */
    long diff = lost->timestamp_minutes - found->timestamp_minutes;
    if (diff < 0) diff = -diff;
    int within_week = (diff <= MATCH_WEEK_MINUTES) ? 1 : 0;

    /* 4. Description keyword tokens overlap with synonym canonicalization */
    char l_tokens[32][32];
    char f_tokens[32][32];
    int l_count = extract_keywords_c(lost->description, l_tokens, 32);
    int f_count = extract_keywords_c(found->description, f_tokens, 32);
    int shared = count_shared_keywords_c(l_tokens, l_count, f_tokens, f_count);

    /* 5. Compute total score and flags */
    int flags = 0;
    res.score = score_pair_spatial_c(same_cat, loc_score, within_week, shared, &flags);
    res.why_flags = flags;
    res.shared_words = shared;
    res.percent = percent_of_c(res.score);
    res.is_match = is_match_c(res.score);

    return res;
}

#ifndef NO_MAIN_TEST
/**
 * Standalone CLI test harness for college project evaluation and demonstrations.
 */
int main(void) {
    printf("===========================================================================\n");
    printf("    COLLEGE LOST & FOUND - C SMART SIMILARITY MATCHING ENGINE              \n");
    printf("      (Lexical Synonyms + 2D Spatial Distance & Radius Decay Engine)       \n");
    printf("===========================================================================\n\n");

    Item lost_items[6] = {
        {"L01", "Accessories", "Library 2nd Floor", "Black specs with matte frame", 10000, 100, 100},
        {"L02", "Electronics", "Library 2nd Floor", "Lost my mobile cellphone with clear case", 10000, 100, 100},
        {"L03", "Bags",        "CC Hall",           "Blue Fastrack college backpack with water bottle", 20000, 50, 240},
        {"L04", "Keys",        "Block C Ground",    "Set of bike keys with Honda metal keychain", 30000, 190, 300},
        {"L05", "Stationery",  "Block B",           "Scientific calculator Casio fx-991EX", 40000, 320, 270},
        {"L06", "Earphones",   "Block B",           "White wireless airpods dropped near corridor", 50000, 308, 262}
    };

    Item found_items[6] = {
        {"F01", "Accessories", "Library 2nd Floor", "Found spectacles with black frame on table", 10200, 110, 110},
        {"F02", "Electronics", "Library 2nd Floor", "Black smartphone found near charging desk", 10200, 105, 108},
        {"F03", "Bags",        "CC Hall",           "Blue Fastrack backpack handed in by student", 21500, 55, 235},
        {"F04", "Keys",        "Block C Ground",    "Silver keys with Honda keychain", 30400, 192, 305},
        {"F05", "Bottles",     "Cafeteria",         "Green Milton steel water bottle", 48000, 40, 50},
        {"F06", "Earphones",   "Block C Walkway",   "Found white earbuds on walkway bench", 50200, 260, 280}
    };

    printf("%-6s | %-12s | %-6s | %-7s | %-12s | %s\n", "PAIR", "CATEGORY", "SCORE", "PERCENT", "DECISION", "REASONS & SPATIAL GEOMETRY");
    printf("-------------------------------------------------------------------------------------------\n");

    for (int i = 0; i < 6; i++) {
        MatchResult r = evaluate_match_c(&lost_items[i], &found_items[i]);
        char reasons[160] = "";

        if (r.why_flags & WHY_SAME_CATEGORY)   strcat(reasons, "Category(+3) ");
        if (r.why_flags & WHY_SAME_LOCATION)   strcat(reasons, "Spot(+2) ");
        if (r.why_flags & WHY_NEARBY_LOCATION) strcat(reasons, "NearbyDecay(+1) ");
        if (r.why_flags & WHY_WITHIN_WEEK)     strcat(reasons, "7-Days(+1) ");
        if (r.why_flags & WHY_KEYWORD_MATCH)   strcat(reasons, "Synonyms/Words ");

        printf("Pair %d | %-12s | %2d/12 | %5d%%  | %-12s | %s\n",
            i + 1,
            lost_items[i].category,
            r.score,
            r.percent,
            r.is_match ? ">> MATCH <<" : "No match",
            reasons);
    }

    printf("-------------------------------------------------------------------------------------------\n");
    printf("Pair 6 Highlights: Lost at Block B (308, 262), Found at Block C Walkway (260, 280).\n");
    printf("Distance squared = (308-260)^2 + (262-280)^2 = 48^2 + (-18)^2 = 2304 + 324 = 2628 <= 16900.\n");
    printf("Result: Spatial Radius Decay engine awards +1 Proximity point! With synonyms, score is 9/12 (75%%)!\n\n");

    return 0;
}
#endif
