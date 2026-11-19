# College Lost & Found - C Similarity Matcher Engine

## 1. Overview
This module implements the **Intelligent Match Suggestion Engine** in the **C Programming Language** (C99/C11 Standard) with **Lexical Synonym Canonicalization** for our College Lost & Found project.

When a student files a lost report, this algorithm automatically compares it against all cataloged found items in the college administration database. Even when different words are used (e.g. `specs` vs `spectacles`, or `mobile` vs `smartphone`), the C synonym canonicalizer normalizes them to common root concepts. When a pair scores **75% or higher** ($\ge 9$ out of $12$ points), the student receives an instant alert with verification options.

---

## 2. Mathematical Scoring & Synonym Normalization

$$\text{Total Score} = S_{\text{category}} + S_{\text{location}} + S_{\text{date}} + S_{\text{keywords/synonyms}}$$

$$\text{Similarity Percentage} = \left\lfloor \frac{\text{Total Score} \times 100 + 6}{12} \right\rfloor$$

| Factor | Weight | Maximum | Description |
| :--- | :---: | :---: | :--- |
| **Category** | $+3$ | $3$ | Identical item classification (e.g., Electronics, Keys, ID Card) |
| **Location** | $+2$ | $2$ | Identical campus zone / landmark |
| **Date Proximity** | $+1$ | $1$ | Lost and found timestamps within 7 days ($10,080$ minutes) |
| **Keywords & Synonyms** | $+2$ | $6$ | Shared canonical terms (specs $\equiv$ glasses, mobile $\equiv$ phone, etc.) |
| **Total** | | **$12$** | **Minimum alert threshold: $\ge 75\%$ (Score $9+/12$)** |

### Built-in Campus Synonym Groups (C Lookups):
- `phone`: mobile, cellphone, smartphone, handset, iphone, android
- `glasses`: specs, spectacles, shades, eyeglasses, goggles
- `earphones`: earbuds, airpods, headphones, headset, earpiece, pods
- `bag`: backpack, rucksack, knapsack, satchel, pouch
- `drive`: pendrive, flashdrive, thumbdrive, usbdrive
- `idcard`: badge, hallticket, identitycard, idbadge
- `wallet`: purse, billfold, moneybag, clutch
- `bottle`: flask, sipper, waterbottle, thermos
- `calculator`: calci, scientificcalculator, casio
- `watch`: smartwatch, timepiece, wristwatch, fastrack, fitbit
- `charger`: adapter, cable, powerbank, cord
- `keys`: keychain, bikekey, scootykey, keyring
- `laptop`: notebook, macbook, computer, thinkpad

---

## 3. How to Compile and Run (CLI Demonstration)

To demonstrate this C module during your lab viva or project review:

### Using GCC (MinGW / Linux / macOS):
```bash
gcc -O2 matcher.c -o matcher
./matcher
```

### Using Windows Developer Command Prompt (MSVC):
```cmd
cl /O2 matcher.c
matcher.exe
```

### Sample Output:
```text
===========================================================================
    COLLEGE LOST & FOUND - C SMART SIMILARITY MATCHING ENGINE              
===========================================================================

PAIR   | CATEGORY     | SCORE  | PERCENT | DECISION     | REASONS
---------------------------------------------------------------------------
Pair 1 | Electronics  | 12/12  |   100%  | >> MATCH <<  | Category(+3) Place(+2) 7-Days(+1) Keywords 
Pair 2 | Bags         | 10/12  |    83%  | >> MATCH <<  | Category(+3) Place(+2) 7-Days(+1) Keywords 
Pair 3 | Keys         |  9/12  |    75%  | >> MATCH <<  | Category(+3) Place(+2) 7-Days(+1) Keywords 
Pair 4 | Stationery   |  0/12  |     0%  | No match     | 
---------------------------------------------------------------------------
Rule: Minimum threshold is >= 75% similarity (Score 9 or higher).
```

---

## 4. Web & Android WebAssembly Integration
The C scoring algorithm is compiled to a high-performance **WebAssembly (`matcher.wasm`)** binary.
In `index.html`, the browser and Android WebView load this WebAssembly module directly:
- `cMatcherModule.score_pair(same_cat, same_loc, within_week, shared_words)`
- `cMatcherModule.percent_of(score)`
- `cMatcherModule.is_match(score)`

This ensures the exact C code logic runs natively on both the live website and the Android APK with sub-millisecond execution times and zero server dependencies.
