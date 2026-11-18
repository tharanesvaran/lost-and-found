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

#include "matcher.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#if defined(_WIN32) && !defined(strcasecmp)
#define strcasecmp _stricmp
#endif

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
 * Helper: Extracts distinct words with 4 or more letters from a text string.
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
            if (buf_idx >= 4) { /* Only words with 4 or more letters */
                buf[buf_idx] = '\0';
                /* Check if token is already present */
                int exists = 0;
                for (int j = 0; j < token_count; j++) {
                    if (strcmp(tokens[j], buf) == 0) {
                        exists = 1;
                        break;
                    }
                }
                if (!exists && token_count < max_tokens) {
                    strncpy(tokens[token_count], buf, 31);
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
    printf("===========================================================================\n\n");

    Item lost_items[4] = {
        {"L01", "Electronics", "Library 2nd Floor", "Black Apple iPhone 13 with clear case", 10000},
        {"L02", "Bags", "CC Hall", "Blue Fastrack college backpack with water bottle", 20000},
        {"L03", "Keys", "Block C Ground", "Set of bike keys with Honda metal keychain", 30000},
        {"L04", "Stationery", "Block B", "Scientific calculator Casio fx-991EX", 40000}
    };

    Item found_items[4] = {
        {"F01", "Electronics", "Library 2nd Floor", "Black Apple iPhone 13 found near charging desk", 10200},
        {"F02", "Bags", "CC Hall", "Blue Fastrack backpack handed in by student", 21500},
        {"F03", "Keys", "Block C Ground", "Silver keys with Honda keychain", 30400},
        {"F04", "Bottles", "Cafeteria", "Green Milton steel water bottle", 48000}
    };

    printf("%-6s | %-12s | %-6s | %-7s | %-12s | %s\n", "PAIR", "CATEGORY", "SCORE", "PERCENT", "DECISION", "REASONS");
    printf("---------------------------------------------------------------------------\n");

    for (int i = 0; i < 4; i++) {
        MatchResult r = evaluate_match_c(&lost_items[i], &found_items[i]);
        char reasons[128] = "";

        if (r.why_flags & WHY_SAME_CATEGORY) strcat(reasons, "Category(+3) ");
        if (r.why_flags & WHY_SAME_LOCATION) strcat(reasons, "Place(+2) ");
        if (r.why_flags & WHY_WITHIN_WEEK)   strcat(reasons, "7-Days(+1) ");
        if (r.why_flags & WHY_KEYWORD_MATCH) strcat(reasons, "Keywords ");

        printf("Pair %d | %-12s | %2d/12 | %5d%%  | %-12s | %s\n",
            i + 1,
            lost_items[i].category,
            r.score,
            r.percent,
            r.is_match ? ">> MATCH <<" : "No match",
            reasons);
    }

    printf("---------------------------------------------------------------------------\n");
    printf("Rule: Minimum threshold is >= 75%% similarity (Score 9 or higher).\n");
    printf("Compiled C WebAssembly binary (matcher.wasm) executes this algorithm\n");
    printf("directly inside the live web and Android WebView application.\n\n");

    return 0;
}
#endif
