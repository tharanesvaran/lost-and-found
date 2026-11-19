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
 *   - Same Campus Zone:    +2 points
 *   - Found within 7 days: +1 point
 *   - Shared 4+ ltr words: +2 points per word (capped at 3 words = max 6 points)
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
#define WEIGHT_LOCATION 2
#define WEIGHT_DATE_PROXIMITY 1
#define WEIGHT_SHARED_WORD 2
#define MAX_SHARED_WORDS_SCORED 3

/* Bitmask flags explaining why an item matched */
#define WHY_SAME_CATEGORY    (1 << 0) /* 1: Same category (+3) */
#define WHY_SAME_LOCATION    (1 << 1) /* 2: Same campus zone (+2) */
#define WHY_WITHIN_WEEK      (1 << 2) /* 4: Found within 7 days (+1) */
#define WHY_KEYWORD_MATCH    (1 << 3) /* 8: Has matching keywords (+2 per word, max 6) */

/* Structure representing an item (either reported lost or logged as found) */
typedef struct {
    char id[64];
    char category[32];
    char location[64];
    char description[256];
    long timestamp_minutes; /* Minutes since Unix epoch */
} Item;

/* Structure representing similarity evaluation outcome */
typedef struct {
    int score;         /* 0 to 12 */
    int percent;       /* 0 to 100% */
    int why_flags;     /* Bitmask of reasons (WHY_*) */
    int shared_words;  /* Count of common keywords */
    int is_match;      /* 1 if percent >= 75 (score >= 9), 0 otherwise */
} MatchResult;

/* Structure for campus synonym mapping / lexical canonicalization */
typedef struct {
    const char* canonical;        /* Canonical root term (e.g. "glasses") */
    const char* aliases[8];       /* Common synonyms (e.g. "specs", "spectacles") */
    int alias_count;
} SynonymGroup;

int score_pair_c(int same_cat, int same_loc, int within_week, int shared_words, int* out_why_flags);
int percent_of_c(int score);
int is_match_c(int score);
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
 * Calculates raw score (0-12) and generates bitmask reason flags.
 */
int score_pair_c(int same_cat, int same_loc, int within_week, int shared_words, int* out_why_flags) {
    int s = 0;
    int w = 0;

    if (same_cat) {
        s += WEIGHT_CATEGORY;
        w |= WHY_SAME_CATEGORY;
    }
    if (same_loc) {
        s += WEIGHT_LOCATION;
        w |= WHY_SAME_LOCATION;
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
    w |= (shared << 4); /* Store count of matching words in upper bits */

    if (out_why_flags) {
        *out_why_flags = w;
    }
    return s;
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
            if (buf_idx >= 3) { /* Minimum 3 letters */
                buf[buf_idx] = '\0';
                /* Canonicalize token using the C synonym dictionary */
                const char* canon = get_canonical_term_c(buf);
                /* Check if token is already present */
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
 * Evaluates similarity between a lost report and a found item.
 */
MatchResult evaluate_match_c(const Item* lost, const Item* found) {
    MatchResult res;
    memset(&res, 0, sizeof(res));

    /* 1. Category check */
    int same_cat = (strcasecmp(lost->category, found->category) == 0) ? 1 : 0;

    /* 2. Campus zone check */
    int same_loc = (strcasecmp(lost->location, found->location) == 0) ? 1 : 0;

    /* 3. Time proximity check (within 7 days = 10,080 minutes) */
    long diff = lost->timestamp_minutes - found->timestamp_minutes;
    if (diff < 0) diff = -diff;
    int within_week = (diff <= MATCH_WEEK_MINUTES) ? 1 : 0;

    /* 4. Description keyword tokens overlap */
    char l_tokens[32][32];
    char f_tokens[32][32];
    int l_count = extract_keywords_c(lost->description, l_tokens, 32);
    int f_count = extract_keywords_c(found->description, f_tokens, 32);
    int shared = count_shared_keywords_c(l_tokens, l_count, f_tokens, f_count);

    /* 5. Compute total score and flags using core formula */
    int flags = 0;
    res.score = score_pair_c(same_cat, same_loc, within_week, shared, &flags);
    res.why_flags = flags;
    res.shared_words = shared;
    res.percent = percent_of_c(res.score);
    res.is_match = is_match_c(res.score);

    return res;
}

#ifndef NO_MAIN_TEST
/**
 * Standalone CLI test harness for college project evaluation and demonstrations.
 * Compile: gcc matcher.c -o matcher
 * Run:     ./matcher
 */
int main(void) {
    printf("===========================================================================\n");
    printf("    COLLEGE LOST & FOUND - C SMART SIMILARITY MATCHING ENGINE              \n");
    printf("      (With Lexical Synonym Canonicalization: specs <-> spectacles)        \n");
    printf("===========================================================================\n\n");

    Item lost_items[5] = {
        {"L01", "Accessories", "Library 2nd Floor", "Black specs with matte frame", 10000},
        {"L02", "Electronics", "Library 2nd Floor", "Lost my mobile cellphone with clear case", 10000},
        {"L03", "Bags", "CC Hall", "Blue Fastrack college backpack with water bottle", 20000},
        {"L04", "Keys", "Block C Ground", "Set of bike keys with Honda metal keychain", 30000},
        {"L05", "Stationery", "Block B", "Scientific calculator Casio fx-991EX", 40000}
    };

    Item found_items[5] = {
        {"F01", "Accessories", "Library 2nd Floor", "Found spectacles with black frame on table", 10200},
        {"F02", "Electronics", "Library 2nd Floor", "Black smartphone found near charging desk", 10200},
        {"F03", "Bags", "CC Hall", "Blue Fastrack backpack handed in by student", 21500},
        {"F04", "Keys", "Block C Ground", "Silver keys with Honda keychain", 30400},
        {"F05", "Bottles", "Cafeteria", "Green Milton steel water bottle", 48000}
    };

    printf("%-6s | %-12s | %-6s | %-7s | %-12s | %s\n", "PAIR", "CATEGORY", "SCORE", "PERCENT", "DECISION", "REASONS & SYNONYMS");
    printf("-------------------------------------------------------------------------------------------\n");

    for (int i = 0; i < 5; i++) {
        MatchResult r = evaluate_match_c(&lost_items[i], &found_items[i]);
        char reasons[128] = "";

        if (r.why_flags & WHY_SAME_CATEGORY) strcat(reasons, "Category(+3) ");
        if (r.why_flags & WHY_SAME_LOCATION) strcat(reasons, "Place(+2) ");
        if (r.why_flags & WHY_WITHIN_WEEK)   strcat(reasons, "7-Days(+1) ");
        if (r.why_flags & WHY_KEYWORD_MATCH) strcat(reasons, "Keywords/Synonyms ");

        printf("Pair %d | %-12s | %2d/12 | %5d%%  | %-12s | %s\n",
            i + 1,
            lost_items[i].category,
            r.score,
            r.percent,
            r.is_match ? ">> MATCH <<" : "No match",
            reasons);
    }

    printf("-------------------------------------------------------------------------------------------\n");
    printf("Rule: Minimum threshold is >= 75%% similarity (Score 9 or higher).\n");
    printf("Synonym canonicalization allows 'specs' to match 'spectacles' and 'mobile' to match 'smartphone'!\n\n");

    return 0;
}
#endif
