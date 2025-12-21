#include "Application.h"

#include <iostream>
#include <fstream>
#include <thread>
#include <chrono>
#include <stdexcept>
#include <cmath>

template <typename TP>
static std::chrono::milliseconds to_ms(TP tp) {
    return std::chrono::duration_cast<std::chrono::milliseconds>(tp);
}

int Application::run(int argc, char **argv) {
    try {
        Config cfg = parseArgs(argc, argv);

        //from where to read inputFile or stdin
        std::istream *in = &std::cin;
        std::ifstream file;
        if (!cfg.inputFile.empty()) {
            file.open(cfg.inputFile.c_str());
            if (!file) {
                throw std::runtime_error("Cannot open file: " + cfg.inputFile);
            }
            in = &file;
        }

        //read matrix
        Matrix m = Matrix::readFrom(*in);

        DeterminantCalculator calc;
        using clock_t = std::chrono::high_resolution_clock;

        std::cout.setf(std::ios::fixed);
        std::cout.precision(10);

        if (cfg.algo == Algo::SEQ) {
            auto start = clock_t::now();
            DetResult r = calc.computeSequential(m);
            auto end = clock_t::now();
            auto ms = to_ms(end - start).count();

            printResult("seq", r);
            std::cout << "Time (seq): " << ms << " ms\n";
        } else if (cfg.algo == Algo::PAR) {
            int threads = cfg.threads;
            if (threads == 0) {
                threads = (int)std::thread::hardware_concurrency();
                if (threads <= 0) threads = 2;
            }

            auto start = clock_t::now();
            DetResult r = calc.computeParallel(m, threads);
            auto end = clock_t::now();
            auto ms = to_ms(end - start).count();

            printResult("par", r);
            std::cout << "Time (par): " << ms << " ms\n";
        } else { // BOTH
            auto start1 = clock_t::now();
            DetResult r1 = calc.computeSequential(m);
            auto end1 = clock_t::now();
            auto ms1 = to_ms(end1 - start1).count();

            int threads = cfg.threads;
            if (threads == 0) {
                threads = (int)std::thread::hardware_concurrency();
                if (threads <= 0) threads = 2;
            }

            auto start2 = clock_t::now();
            DetResult r2 = calc.computeParallel(m, threads);
            auto end2 = clock_t::now();
            auto ms2 = to_ms(end2 - start2).count();

            printResult("seq", r1);
            std::cout << "Time (seq): " << ms1 << " ms\n\n";

            printResult("par", r2);
            std::cout << "Time (par): " << ms2 << " ms\n\n";

            //compare results from both algorithms
            if (r1.sign == 0 && r2.sign == 0) {
                std::cout << "Both determinants are zero.\n";
            } else if (r1.sign != r2.sign) {
                std::cout << "WARNING: determinants have different sign!\n";
            } else {
                long double diff = std::fabs(r1.logAbs - r2.logAbs);
                std::cout << "Absolute difference between log10(|det|): "
                          << (double)diff << "\n";
            }
        }

        return 0;
    } catch (const std::exception &e) {
        std::cerr << "Error: " << e.what() << "\n";
        std::cerr << "Use --help for usage.\n";
        return 1;
    }
}

Application::Config Application::parseArgs(int argc, char **argv) {
    Config cfg;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];

        if (arg == "--help") {
            printHelp(argv[0]);
            std::exit(0);
        } else if (arg == "--algo") {
            if (i + 1 >= argc) {
                throw std::runtime_error("Missing value after --algo");
            }
            std::string mode = argv[++i];
            if (mode == "seq")       cfg.algo = Algo::SEQ;
            else if (mode == "par")  cfg.algo = Algo::PAR;
            else if (mode == "both") cfg.algo = Algo::BOTH;
            else throw std::runtime_error("Unknown algo mode: " + mode);
        } else if (arg == "--input") {
            if (i + 1 >= argc) {
                throw std::runtime_error("Missing value after --input");
            }
            cfg.inputFile = argv[++i];
        } else if (arg == "--threads") {
            if (i + 1 >= argc) {
                throw std::runtime_error("Missing value after --threads");
            }
            cfg.threads = std::stoi(argv[++i]);
            if (cfg.threads <= 0) {
                throw std::runtime_error("Threads must be > 0");
            }
        } else if (!arg.empty() && arg[0] == '-') {
            throw std::runtime_error("Unknown switch: " + arg);
        } else {
            if (!cfg.inputFile.empty()) {
                throw std::runtime_error("Multiple input files");
            }
            cfg.inputFile = arg;
        }
    }

    return cfg;
}

void Application::printHelp(const char *progName) {
    std::cout
        << "Usage: " << progName
        << " --algo {seq|par|both} [--threads N] [--input FILE]\n\n"
        << "Computes determinant of a square matrix.\n"
        << "Result is printed as sign and log10(|det|).\n"
        << "If FILE is not given, matrix is read from standard input.\n\n"
        << "Options:\n"
        << "  --help          Show this help and exit.\n"
        << "  --algo MODE     seq | par | both\n"
        << "  --threads N     Number of threads for par (default: HW concurrency)\n"
        << "  --input FILE    Read matrix from FILE\n";
}

void Application::printResult(const std::string &tag, const DetResult &r) {
    if (r.sign == 0) {
        std::cout << "Determinant (" << tag << "): 0\n";
    } else {
        std::cout << "Determinant (" << tag << "): sign=" << r.sign
                  << ", log10(|det|)=" << (double)r.logAbs << "\n";
    }
}
