#ifndef ALGORITHM_APPLICATION_H
#define ALGORITHM_APPLICATION_H

#include <string>

#include "Matrix.h"
#include "DeterminantCalculator.h"

// Application class:
//  parses command line arguments
//  reads matrix from file / stdin
//  runs selected algorithm (sequential / parallel / both)
//  prints results and timing
class Application {
public:
    int run(int argc, char **argv);

private:
    //which algorithm to run
    enum class Algo { SEQ, PAR, BOTH };
    //parsed command configuration
    struct Config {
        Algo algo;
        std::string inputFile;
        int threads; //0 = auto

        Config() : algo(Algo::SEQ), inputFile(), threads(0) {}
    };

    Config parseArgs(int argc, char **argv);
    void printHelp(const char *progName);
    void printResult(const std::string &tag, const DetResult &r);
};

#endif // ALGORITHM_APPLICATION_H
