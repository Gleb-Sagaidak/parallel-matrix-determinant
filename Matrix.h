#ifndef ALGORITHM_MATRIX_H
#define ALGORITHM_MATRIX_H

#include <vector>
#include <istream>
#include <stdexcept>

//сlass representing a square matrix of size n x n.
class Matrix {
    public:
        Matrix() : n_(0) {}

        explicit Matrix(int n) : n_(n), data_(n * n, 0.0L) {}

        int size() const {
            return n_;
        }

        long double &operator()(int i, int j) {
            return data_[i * n_ + j];
        }

        long double operator()(int i, int j) const {
            return data_[i * n_ + j];
        }
        // Swap two rows of the matrix
        void swapRows(int r1, int r2) {
            if (r1 == r2) return;
            for (int j = 0; j < n_; ++j) {
                std::swap((*this)(r1, j), (*this)(r2, j));
            }
        }

        //read matrix from input stream.
        //expected format:
        //   n
        //   a11 a12 ... a1n
        //   ...
        //   an1 an2 ... ann
        static Matrix readFrom(std::istream &in) {
            int n;
            if (!(in >> n)) {
                throw std::runtime_error("Cannot read matrix size");
            }
            Matrix m(n);
            for (int i = 0; i < n; ++i) {
                for (int j = 0; j < n; ++j) {
                    if (!(in >> m(i, j))) {
                        throw std::runtime_error("Cannot read matrix element");
                    }
                }
            }
            return m;
        }
    private:
        int n_;
        std::vector<long double> data_;
};

//result of solving determinant
//it doesn't store det(A) directly to avoid overflow
//it store det as log10(|det(A)|)
//variable sign store a sign (-1,0,1) of the det(A)
struct DetResult {
    int sign;
    long double logAbs;
};

#endif