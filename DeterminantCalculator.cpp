#include "DeterminantCalculator.h"
#include <cmath>
#include <thread>

DetResult DeterminantCalculator::computeSequential(Matrix a) const {
    const long double eps = 1e-18L;
    int n = a.size();
    if (n == 0) return {1, 0.0L};

    int sign = 1;

    for (int k = 0; k < n; ++k) {
        //find pivot row in column k (max |A(i,k)|, i>=k)
        int piv = k;
        long double best = std::fabs(a(k, k));
        for (int i = k + 1; i < n; ++i) {
            long double v = std::fabs(a(i, k));
            if (v > best) {
                best = v;
                piv = i;
            }
        }

        if (best < eps) return {0, 0.0L};

        //swap rows if necessary
        if (piv != k) {
            a.swapRows(k, piv);
            sign = -sign;
        }

        long double pivot = a(k, k);

        // Eliminate entries below the pivot in column k
        for (int i = k + 1; i < n; ++i) {
            long double factor = a(i, k) / pivot;
            a(i, k) = 0.0L;
            for (int j = k + 1; j < n; ++j) {
                a(i, j) -= factor * a(k, j);
            }
        }
    }


    //now matrix is upper-triangular. det(A) = sign * product of diagonal.
    long double logAbs = 0.0L;
    for (int i = 0; i < n; ++i) {
        long double d = a(i, i);
        long double ad = std::fabs(d);
        if (ad < eps) return {0, 0.0L};
        if (d < 0.0L) sign = -sign;
        logAbs += std::log10(ad);
    }
    return {sign, logAbs};
}

DetResult DeterminantCalculator::computeParallel(Matrix a, int num_threads) const {
    const long double eps = 1e-18L;
    int n = a.size();
    if (n == 0) return {1, 0.0L};
    if (num_threads < 1) num_threads = 1;
    int sign = 1;

    for (int k = 0; k < n; ++k) {
        //find pivot in column k
        int piv = k;
        long double best = std::fabs(a(k, k));
        for (int i = k + 1; i < n; ++i) {
            long double v = std::fabs(a(i, k));
            if (v > best) {
                best = v;
                piv = i;
            }
        }
        if (best < eps) return {0, 0.0L};

        if (piv != k) {
            a.swapRows(k, piv);
            sign = -sign;
        }
        long double pivot = a(k, k);
        int rowsLeft = n - (k + 1);
        if (rowsLeft <= 0) break;

        //we cannot have more threads than rows to process
        int useThreads = std::min(rowsLeft, num_threads);

        if (useThreads == 1) {
            for (int i = k + 1; i < n; ++i) {
                long double factor = a(i, k) / pivot;
                a(i, k) = 0.0L;
                for (int j = k + 1; j < n; ++j) {
                    a(i, j) -= factor * a(k, j);
                }
            }
        } else {
            //split rows [k+1 .. n-1] into chunks for each thread
            int chunk = (rowsLeft + useThreads - 1) / useThreads;
            std::vector<std::thread> threads;
            int nLocal = n;

            for (int t = 0; t < useThreads; ++t) {
                int startRow = k + 1 + t * chunk;
                if (startRow >= nLocal) break;
                int endRow = std::min(nLocal, startRow + chunk);

                //each thread processes its own block of rows
                threads.emplace_back(
                    [&a, nLocal, k, pivot, startRow, endRow]() {
                        for (int i = startRow; i < endRow; ++i) {
                            long double factor = a(i, k) / pivot;
                            a(i, k) = 0.0L;
                            for (int j = k + 1; j < nLocal; ++j) {
                                a(i, j) -= factor * a(k, j);
                            }
                        }
                    }
                );
            }
            //wait for all threads to finish step k
            for (auto &th : threads) th.join();
        }
    }
    long double logAbs = 0.0L;
    for (int i = 0; i < n; ++i) {
        long double d = a(i, i);
        long double ad = std::fabs(d);
        if (ad < eps) return {0, 0.0L};
        if (d < 0.0L) sign = -sign;
        logAbs += std::log10(ad);
    }
    return {sign, logAbs};
}