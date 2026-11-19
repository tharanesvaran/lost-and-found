# College Lost & Found - C Similarity Matcher Engine

## 1. Overview
This module implements the **Intelligent Match Suggestion Engine** in the **C Programming Language** (C99/C11 Standard) with:
1. **Lexical Synonym Canonicalization** (specs $\leftrightarrow$ spectacles $\rightarrow$ glasses, mobile $\leftrightarrow$ smartphone $\rightarrow$ phone)
2. **2D Euclidean Spatial Distance & Campus Radius Decay Engine** ($540 \times 440$ interactive campus map geometry)

When a student files a lost report, this algorithm automatically compares it against all cataloged found items in the college administration database. Even when different words are used or when an item was picked up slightly away from where it was dropped (e.g. adjacent walkway or nearby building), the spatial geometry and synonym canonicalizer compute the mathematical proximity. When a pair scores **75% or higher** ($\ge 9$ out of $12$ points), the student receives an instant alert with verification options.

---

## 2. Mathematical Scoring, Spatial Geometry & Synonym Normalization

$$\text{Total Score} = S_{\text{category}} + S_{\text{spatial}} + S_{\text{date}} + S_{\text{keywords/synonyms}}$$

$$\text{Similarity Percentage} = \left\lfloor \frac{\text{Total Score} \times 100 + 6}{12} \right\rfloor$$

| Factor | Weight | Maximum | Description |
| :--- | :---: | :---: | :--- |
| **Category** | $+3$ | $3$ | Identical item classification (e.g., Electronics, Keys, ID Card) |
| **Spatial Proximity** | $+2$ or $+1$ | $2$ | **Spot Match** ($d \le 45\text{px} \implies d^2 \le 2025$): $+2$ pts<br>**Nearby Decay** ($45 < d \le 130\text{px} \implies d^2 \le 16900$): $+1$ pt<br>Distant ($d > 130\text{px}$): $+1$ pt if same zone, else $0$<br>Unpinned fallback: $+2$ pts if same zone, else $0$ |
| **Date Proximity** | $+1$ | $1$ | Lost and found timestamps within 7 days ($10,080$ minutes) |
| **Keywords & Synonyms** | $+2$ | $6$ | Shared canonical terms (specs $\equiv$ glasses, mobile $\equiv$ phone, etc.) |
| **Total** | | **$12$** | **Minimum alert threshold: $\ge 75\%$ (Score $9+/12$)** |

### Spatial Distance Math (Integer Arithmetic):
$$\Delta x = x_1 - x_2, \quad \Delta y = y_1 - y_2$$
$$d^2 = (\Delta x)^2 + (\Delta y)^2$$
- Squared distance avoids square root operations, ensuring $O(1)$ constant single-cycle time with zero floating-point roundoff errors.

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
      (Lexical Synonyms + 2D Spatial Distance & Radius Decay Engine)       
===========================================================================

PAIR   | CATEGORY     | SCORE  | PERCENT | DECISION     | REASONS & SPATIAL GEOMETRY
-------------------------------------------------------------------------------------------
Pair 1 | Accessories  | 10/12  |    83%  | >> MATCH <<  | Category(+3) Spot(+2) 7-Days(+1) Synonyms/Words 
Pair 2 | Electronics  | 10/12  |    83%  | >> MATCH <<  | Category(+3) Spot(+2) 7-Days(+1) Synonyms/Words 
Pair 3 | Bags         | 10/12  |    83%  | >> MATCH <<  | Category(+3) Spot(+2) 7-Days(+1) Synonyms/Words 
Pair 4 | Keys         | 10/12  |    83%  | >> MATCH <<  | Category(+3) Spot(+2) 7-Days(+1) Synonyms/Words 
Pair 5 | Bottles      |  0/12  |     0%  | No match     | 
Pair 6 | Earphones    |  9/12  |    75%  | >> MATCH <<  | Category(+3) NearbyDecay(+1) 7-Days(+1) Synonyms/Words 
-------------------------------------------------------------------------------------------
Pair 6 Highlights: Lost at Block B (308, 262), Found at Block C Walkway (260, 280).
Distance squared = (308-260)^2 + (262-280)^2 = 48^2 + (-18)^2 = 2304 + 324 = 2628 <= 16900.
Result: Spatial Radius Decay engine awards +1 Proximity point! With synonyms, score is 9/12 (75%)!
```

---

## 4. Web & Android WebAssembly Integration
The C scoring algorithm is compiled to a high-performance **WebAssembly (`matcher.wasm`)** binary.
In `index.html`, the browser and Android WebView load this WebAssembly module directly:
- `cMatcherExports.score_pair(same_cat, loc_score, within_week, shared_words)`
- `cMatcherExports.percent_of(score)`
- `cMatcherExports.is_match(score)`
- `cMatcherExports.spatial_score(x1, y1, x2, y2, same_zone)`
- `cMatcherExports.spatial_dist_sq(x1, y1, x2, y2)`

This ensures the exact C code logic runs natively on both the live website and the Android APK with sub-millisecond execution times and zero server dependencies.
