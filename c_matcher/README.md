# College Lost & Found - C Algorithmic Engine Suite

## 1. Overview
This module implements the complete **C Native Algorithmic Engine** (C99/C11 Standard) for the College Lost & Found System.

It combines multi-factor similarity matching, computational geometry, string distance metrics, and state machines into a unified, high-performance C architecture.

---

## 2. Core Functional Modules in C

### Module 1: Similarity Scoring & Threshold Evaluation
- **Formula**:
  $$\text{Total Score} = S_{\text{category}}(3) + S_{\text{spatial}}(2) + S_{\text{date}}(1) + S_{\text{keywords}}(6) = 12 \text{ pts max}$$
- **Threshold**: $\ge 75\%$ ($\ge 9/12$) triggers student match alerts.
- **Exported Functions**: `score_pair_c()`, `score_pair_spatial_c()`, `percent_of_c()`, `is_match_c()`.

### Module 2: Lexical Synonym Canonicalization
- Resolves campus vocabulary mismatches in $O(1)$ time:
  - `specs` $\leftrightarrow$ `spectacles` $\rightarrow$ `glasses`
  - `mobile` $\leftrightarrow$ `cellphone` $\rightarrow$ `phone`
  - `airpods` $\leftrightarrow$ `earbuds` $\rightarrow$ `earphones`
  - `calci` $\leftrightarrow$ `casio` $\rightarrow$ `calculator`
- **Function**: `get_canonical_term_c()`.

### Module 3: 2D Spatial Distance & Radius Decay Geometry
- Integer squared Euclidean distance $d^2 = (\Delta x)^2 + (\Delta y)^2$.
  - **Spot Match**: $d^2 \le 2025$ ($d \le 45\text{px}$) $\rightarrow$ $+2$ points.
  - **Nearby Decay**: $2025 < d^2 \le 16900$ ($d \le 130\text{px}$) $\rightarrow$ $+1$ point.
  - **Distant**: $> 130\text{px} \rightarrow 0$ points (or $+1$ if same named zone).
- **Functions**: `calculate_spatial_distance_sq_c()`, `calculate_spatial_score_c()`.

---

## 3. The 6 Extended Features Implemented in C

### Feature 1: Fuzzy Search & Typo-Tolerance Engine (Levenshtein Distance)
- **Algorithm**: Wagner–Fischer Dynamic Programming edit distance matrix.
- **Purpose**: When a student misspells search queries (e.g., `"sammsung"`, `"iphne"`, `"calcilator"`), the C algorithm computes character edit distance ($O(M \times N)$) and converts it to a normalized similarity percentage ($0 \dots 100\%$).
- **Functions**: `levenshtein_distance_c()`, `fuzzy_match_score_c()`.

### Feature 2: Complex 2D Polygon Ray-Casting (Campus Landmark Detection)
- **Algorithm**: Jordan Curve Theorem (Ray-Casting Algorithm).
- **Purpose**: Instead of basic rectangular boxes, campus landmarks (such as L-shaped Block A or hexagonal CC Hall) are represented as true $N$-point 2D polygons. An imaginary horizontal ray is cast from $(x, y)$, counting edge crossings to determine interior placement.
- **Functions**: `point_in_polygon_c()`, `detect_campus_zone_polygon_c()`.

### Feature 3: Anti-Theft Verification Question & Answer Similarity Normalizer
- **Purpose**: When claiming locked valuables (smartphones, wallets, laptops), claimants frequently type answers with slight formatting differences (e.g., `"Matte black case"` vs `"Black matte silicon cover"`).
- **Algorithm**: Normalizes case, tokenizes keywords, and performs bidirectional token matching with $\le 1$ edit distance tolerance to approve rightful claims while rejecting fraudulent attempts.
- **Functions**: `verify_answer_similarity_c()`, `is_answer_accepted_c()`.

### Feature 4: Holding Period Expiration & Legal Disposition State Machine (FSM)
- **Algorithm**: Deterministic state machine governing item retention according to institutional college policies:
  $$\text{Active (Unclaimed)} \xrightarrow{\le 7\text{d left}} \text{Grace Period} \xrightarrow{\text{Expired}} \begin{cases} \text{Auction} & \text{(Valuables: Phones, Laptops)} \\ \text{Donation} & \text{(General: Bags, Books, Clothes)} \\ \text{Archive} & \text{(Sensitive: IDs, ATM cards)} \end{cases}$$
- **Functions**: `evaluate_disposition_state_c()`, `disposition_state_name_c()`.

### Feature 5: Okapi BM25 / Keyword Relevance Search Ranking
- **Purpose**: Ranks cataloged items by query relevance rather than chronological database insertion order.
- **Algorithm**: Evaluates exact title matches, description occurrences, canonicalized campus synonyms, and fuzzy keyword proximity to produce an integer relevance score ($0 \dots 100$).
- **Function**: `compute_search_relevance_c()`.

### Feature 6: Campus Loss Analytics & Statistical Aggregator
- **Purpose**: Aggregates batch logs into institutional administrative metrics for college presentations and reviews.
- **Metrics Computed**:
  - **Recovery Rate %**: $\frac{\text{Returned Items} \times 100}{\text{Found Items}}$
  - **Average Resolution Time (MTTR)**: Turnaround hours from initial report to verified handover.
  - **Highest Risk Campus Zone**: Frequency distribution identifying the campus block with the most lost reports.
- **Function**: `aggregate_campus_analytics_c()`.

---

## 4. How to Compile and Run (CLI Demonstration)

To demonstrate the full C module suite during lab viva, project reviews, or viva voce:

### Using GCC (MinGW / Linux / macOS):
```bash
gcc -O2 matcher.c -o matcher
./matcher
```

### Using MSVC (Visual Studio / Windows Developer Prompt):
```cmd
cl /O2 matcher.c
matcher.exe
```

### Sample Output:
```text
===========================================================================
    COLLEGE LOST & FOUND - FULL C NATIVE ALGORITHMIC ENGINE                
  (Similarity + Synonyms + Spatial + 6 Extended Features)                 
===========================================================================

[MODULE 1] Fuzzy Search & Typo-Tolerance (Levenshtein Distance):
  Query: 'sammsung   ' vs Target: score =  88/100 (Match)
  Query: 'iphne      ' vs Target: score =  83/100 (Match)
  Query: 'calcilator ' vs Target: score =  90/100 (Match)
  Query: 'spectacls  ' vs Target: score =  80/100 (Match)

[MODULE 2] Complex 2D Polygon Ray-Casting (Campus Map Geometry):
  Pin (300, 100) -> Detected Campus Zone: Block A
  Pin (330, 290) -> Detected Campus Zone: Block B
  Pin ( 50, 250) -> Detected Campus Zone: CC Hall

[MODULE 3] Verification Answer Matching (Phonetic & Token Tolerance):
  Student: 'Matte black case ' | Admin: 'Black matte silicon cover' -> 100% (APPROVED)
  Student: 'spiderman blue   ' | Admin: 'Blue Spider-Man case     ' -> 100% (APPROVED)
  Student: 'White clear      ' | Admin: 'Red rubber bumper        ' ->   0% (REJECTED)

[MODULE 4] Holding Expiration & Legal Disposition State Machine (FSM):
  Phone (Day 50/45)    -> State: Eligible for College Auction (days left: -5)
  Bottle (Day 50/45)   -> State: Eligible for Student Charity Donation (days left: -5)
  ID Card (Day 50/45)  -> State: Archived / Destroyed (Confidential) (days left: -5)

[MODULE 5] BM25 / Keyword Relevance Search Ranking:
  Query: 'lost specs glasses'
    Item 1: 'Eyeglasses' -> Relevance:  90/100
    Item 2: 'Keys'       -> Relevance:   0/100

[MODULE 6] Campus Loss Analytics & Statistical Aggregator:
  Total Lost: 4 | Found: 3 | Returned: 2
  Recovery Rate: 66% | Avg Turnaround Time: 22 hrs
  Highest Risk Campus Area: Block B

===========================================================================
 All 6 C Native Engine Modules verified successfully with zero errors!
===========================================================================
```

---

## 5. Web & Mobile WebAssembly Integration
The core mathematical similarity, spatial distance, and synonym functions are compiled into **`matcher.wasm`** and loaded inside `index.html` via `cMatcherExports`:
- `score_pair(c, loc_score, w, s)`
- `spatial_score(x1, y1, x2, y2, same_zone)`
- `spatial_dist_sq(x1, y1, x2, y2)`
- `percent_of(score)`
- `is_match(score)`

This provides native speed, zero server costs, and consistent evaluation across both the web app and the Android WebView APK.
