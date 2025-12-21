#ifndef ALGORITHM_DETERMINANTCALCULATOR_H
#define ALGORITHM_DETERMINANTCALCULATOR_H
#include "Matrix.h"

//class implementing 2 algorithms for solving determinant:
//sequential Gaussian elemination
//multi-thread Gaussian elemination
class DeterminantCalculator {
    public:
    //single-thread GE with partial-pivoting
    DetResult computeSequential(Matrix a) const;

    //multi-thread GE
    DetResult computeParallel(Matrix a, int num_threads) const;
};
#endif