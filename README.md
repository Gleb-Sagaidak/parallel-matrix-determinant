# Parallel Matrix Determinant

Computing the determinant of a square matrix using **Gaussian elimination with partial pivoting**, implemented in both **sequential** and **parallel** versions in C++. Includes benchmark comparisons across matrix sizes from 3×3 to 5000×5000.

> Built as a semester project to practice multithreading with `std::thread`, numerical stability techniques, and performance measurement of parallel algorithms.

---

## What it does

The program reads a square matrix from a file (or stdin) and computes its determinant using Gaussian elimination, then reports:

- the **sign** of the determinant (`-1`, `0`, or `1`),
- the **base-10 logarithm of its absolute value** (`log10(|det|)`),
- the **execution time** in milliseconds.

### Why `log10(|det|)` and not the raw value?

For large matrices the determinant grows extremely fast — for a 5000×5000 matrix it can easily exceed what `long double` can hold and overflow to infinity. Storing the result as `(sign, log10(|det|))` keeps it numerically representable for matrices of any practical size. Internally, instead of computing the product of diagonal elements, the program accumulates the sum of `log10(|A(i,i)|)` and tracks the sign separately.

---

## Algorithm

Gaussian elimination reduces the matrix to upper-triangular form via elementary row operations. Once triangular, the determinant is the product of the diagonal elements (or, equivalently, `sign × 10^(Σ log10(|A(i,i)|))`).

### Partial pivoting

For numerical stability, at each step `k` the algorithm selects the row with the largest `|A(i,k)|` in the current column as the pivot. If the pivot row isn't `k`, the rows are swapped — which flips the sign of the determinant.

### Steps

1. In the current column `k`, find the row with the largest absolute value (the pivot row).
2. If `|A(k,k)| < eps`, the matrix is singular → `det = 0`.
3. If the pivot row isn't already at position `k`, swap them and flip the determinant's sign.
4. Eliminate all entries below the diagonal in column `k` (row reduction).
5. After processing all columns, read off `sign` and `log10(|det|)` from the diagonal.

### Parallel version

The parallel implementation uses identical pivot selection and identical numerical logic — the only difference is **step 4** (row elimination). Rows below the current pivot row are split into contiguous blocks and processed by multiple `std::thread`s in parallel. At each elimination step `k`:

1. Threads are spawned, each handling a block of rows.
2. The main thread `join()`s them before moving to step `k+1`.

The thread count is configurable via `--threads N`; if omitted, the program uses `std::thread::hardware_concurrency()`.

This approach works well for large matrices but has measurable overhead for small ones — see the benchmarks below.

---

## Build

Requires a C++17-capable compiler and CMake 3.10+.

```bash
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
cmake --build .
```

The resulting binary is `algorithm`.

---

## Usage

```bash
./algorithm [OPTIONS]
```

### Options

| Flag              | Description                                                |
|-------------------|------------------------------------------------------------|
| `--help`          | Show help and exit                                         |
| `--algo MODE`     | Choose mode: `seq` \| `par` \| `both`                      |
| `--threads N`     | Number of threads for the parallel run (default: hardware concurrency) |
| `--input FILE`    | Read matrix from a file (otherwise reads from stdin)       |

If `--input` is omitted, the input file can also be passed as the last positional argument, or piped via stdin.

### Input format

```
n
a11 a12 ... a1n
a21 a22 ... a2n
...
an1 an2 ... ann
```

The first line is the matrix dimension `n`; the following `n` lines contain `n` space-separated values each.

### Examples

Sequential run:
```bash
./algorithm --algo seq --input ../data/matrix2000.txt
```

Parallel run with 8 threads:
```bash
./algorithm --algo par --threads 8 --input ../data/matrix2000.txt
```

Compare both versions on the same input:
```bash
./algorithm --algo both --input ../data/matrix5000.txt
```

---

## Architecture

The program is split into three classes with clear responsibilities:

### `Matrix`
Stores the matrix as a 1D array for cache-friendly access; exposes:
- `size()` — matrix dimension
- `operator()(i, j)` — element access
- `swapRows(r1, r2)` — used during pivot selection
- `readFrom(std::istream&)` — parses the input format above

### `DeterminantCalculator`
Holds the two algorithm implementations:
- `computeSequential(Matrix a)` — sequential Gaussian elimination with partial pivoting
- `computeParallel(Matrix a, int num_threads)` — parallel version of step 4 (elimination)

Both return a `DetResult { int sign; long double logAbs; }`.

### `Application`
The CLI layer:
- `parseArgs(argc, argv)` — handles command-line options
- `run()` — loads the matrix, runs the chosen algorithm, and times it
- `printResult(tag, DetResult)` — formats and prints the result

---

## Benchmarks

Testing was done on **6 input matrices** ranging from 3×3 to 5000×5000. In all cases the sequential and parallel results matched exactly — the absolute difference in `log10(|det|)` was `0.0000000000` across the board, confirming correctness.

| Input                   | Size       | Sequential | Parallel  | Speedup  |
|-------------------------|------------|-----------:|----------:|---------:|
| `matrix3.txt`           | 3 × 3      | 0 ms       | 0 ms      | —        |
| `matrix50.txt`          | 50 × 50    | 0 ms       | 11 ms     | **slower** |
| `matrix_1000.txt`       | 1000 × 1000| 931 ms     | 447 ms    | ~2.08×   |
| `matrix1000_big.txt`    | 1000 × 1000| 937 ms     | 453 ms    | ~2.07×   |
| `matrix2000.txt`        | 2000 × 2000| 7 405 ms   | 5 267 ms  | ~1.41×   |
| `matrix5000.txt`        | 5000 × 5000| 113 800 ms | 67 039 ms | ~1.70×   |

### What the numbers show

- **Small matrices (3×3, 50×50):** the parallel version is the same speed or *slower* because thread creation overhead dominates the actual work. For a 50×50 matrix the elimination step has so little work per row that spawning threads each iteration costs more than the work itself.
- **Medium matrices (1000×1000):** the sweet spot for this implementation — about **2× speedup**, close to what one might expect on a multi-core CPU once threading overhead is amortised.
- **Large matrices (2000+):** speedup drops below 2×. The bottleneck shifts from CPU to **memory bandwidth** and the per-iteration cost of spawning/joining threads (the implementation re-creates threads at every elimination step `k`, of which there are `n` total — that's 5 000 spawn/join cycles on the 5000×5000 case).

### Take-away

Parallelisation isn't free — it pays off only when the per-iteration work is large enough to dominate the threading overhead. The current implementation demonstrates the classic speedup curve: useless for small inputs, near-2× on medium ones, sub-linear scaling on very large inputs due to memory and thread-pool effects.

---

## Known limitations & possible improvements

- **Thread pool instead of per-step thread creation.** Currently `std::thread`s are created and joined at every elimination step `k`. For an `n×n` matrix that's `n` spawn/join cycles. Reusing a fixed thread pool (e.g. `std::async` with a custom executor, or a hand-rolled pool with a work queue) would cut the per-step overhead significantly and likely improve the 5000×5000 case.
- **No SIMD vectorisation.** The inner elimination loop is a perfect candidate for SSE/AVX intrinsics or compiler auto-vectorisation hints.
- **No cache blocking.** For very large matrices the access pattern doesn't fit in L2/L3 cache; tiling (block decomposition) would help.
- **Threshold for switching to sequential.** Below ~100×100 the parallel path is consistently slower; the program could detect this and fall back to sequential automatically.

---

## Tech & dependencies

C++17, CMake. **Only the C++ standard library** — no external dependencies.

Standard library headers used: `<iostream>`, `<vector>`, `<fstream>`, `<string>`, `<thread>`, `<chrono>`, `<stdexcept>`, `<cmath>`.

---

## References

- Linear algebra coursework — theory of determinants and Gaussian elimination
- [cppreference.com](https://en.cppreference.com/) — `std::vector`, `std::thread`, I/O streams, `<chrono>`

