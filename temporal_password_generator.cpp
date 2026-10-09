// Temporal Password Generator
// Self-contained C++11 implementation: standard library only.
// The program emits a standalone C++ source file; it does not invoke a shell,
// compiler, or generated executable at runtime.

#include <cerrno>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <random>
#include <string>
#include <vector>

namespace temporal_password {

constexpr int kMinRollers = 5;
constexpr int kMaxRollers = 20;
constexpr std::uint64_t kMinTicks = 1000ULL;
constexpr std::uint64_t kMaxTicks = 100000000000000ULL;
constexpr double kCoupling = 0.3;
constexpr double kPiApprox = 3.14159;
constexpr double kEstimatedUpdatesPerSecond = 11450382.0;

static_assert(sizeof(double) == sizeof(std::uint64_t),
              "This implementation requires 64-bit double.");
static_assert(std::numeric_limits<double>::is_iec559,
              "This implementation requires IEEE-754 double.");

struct GeneratorParams {
    int num_rollers;
    std::uint64_t num_ticks;
    std::vector<double> roller_speeds;
};

bool valid_params(const GeneratorParams& params) {
    return params.num_rollers >= kMinRollers &&
           params.num_rollers <= kMaxRollers &&
           params.num_ticks >= kMinTicks &&
           params.num_ticks <= kMaxTicks &&
           params.roller_speeds.size() ==
               static_cast<std::size_t>(params.num_rollers);
}

std::vector<double> generate_roller_speeds(int num_rollers,
                                           std::uint32_t seed) {
    std::mt19937 engine(seed);
    std::uniform_real_distribution<double> distribution(0.1, 1.0);

    std::vector<double> speeds(static_cast<std::size_t>(num_rollers));
    for (int i = 0; i < num_rollers; ++i) {
        speeds[static_cast<std::size_t>(i)] = distribution(engine);
    }
    return speeds;
}

std::uint32_t make_random_seed() {
    std::random_device source;
    return static_cast<std::uint32_t>(source());
}

std::string calculate_password(int num_rollers,
                               std::uint64_t num_ticks,
                               const std::vector<double>& roller_speeds) {
    if (num_rollers <= 0 ||
        roller_speeds.size() != static_cast<std::size_t>(num_rollers)) {
        return std::string();
    }

    std::vector<double> positions(static_cast<std::size_t>(num_rollers), 0.0);
    std::vector<double> old_positions(static_cast<std::size_t>(num_rollers), 0.0);

    for (std::uint64_t tick = 0; tick < num_ticks; ++tick) {
        for (int i = 0; i < num_rollers; ++i) {
            old_positions[static_cast<std::size_t>(i)] =
                positions[static_cast<std::size_t>(i)];
        }

        for (int i = 0; i < num_rollers; ++i) {
            const int next = (i + 1) % num_rollers;
            double position = old_positions[static_cast<std::size_t>(i)] +
                              roller_speeds[static_cast<std::size_t>(i)];
            position += kCoupling *
                        std::sin(old_positions[static_cast<std::size_t>(next)] *
                                 kPiApprox);
            positions[static_cast<std::size_t>(i)] =
                std::round(position * 100.0) / 100.0;
        }
    }

    std::string password;
    password.reserve(static_cast<std::size_t>(num_rollers));
    std::uint64_t hash = 14695981039346656037ULL;

    for (int char_idx = 0; char_idx < num_rollers; ++char_idx) {
        std::uint64_t position_bits = 0;
        std::memcpy(&position_bits,
                    &positions[static_cast<std::size_t>(char_idx)],
                    sizeof(position_bits));
        hash ^= position_bits;
        hash *= 1099511628211ULL;

        for (int j = 0; j < num_rollers; ++j) {
            if (j != char_idx) {
                std::uint64_t other_bits = 0;
                std::memcpy(&other_bits,
                            &positions[static_cast<std::size_t>(j)],
                            sizeof(other_bits));
                hash ^= other_bits;
                hash *= 1099511628211ULL;
            }
        }

        hash ^= static_cast<std::uint64_t>(char_idx) *
                0x9E3779B97F4A7C15ULL;
        hash *= 1099511628211ULL;

        const int ascii_value = 33 + static_cast<int>(hash % 94ULL);
        password.push_back(static_cast<char>(ascii_value));
    }

    return password;
}

bool write_standalone_source(const GeneratorParams& params,
                             const std::string& filename) {
    if (!valid_params(params)) {
        return false;
    }

    std::ofstream out(filename.c_str(), std::ios::out | std::ios::trunc);
    if (!out) {
        return false;
    }

    out << "// Auto-generated standalone temporal password program.\n"
        << "// Requires only a C++11 standard library implementation.\n"
        << "#include <cmath>\n"
        << "#include <cstddef>\n"
        << "#include <cstdint>\n"
        << "#include <cstring>\n"
        << "#include <iostream>\n"
        << "#include <limits>\n"
        << "#include <string>\n"
        << "#include <vector>\n\n"
        << "static_assert(sizeof(double) == sizeof(std::uint64_t),\n"
        << "              \"Requires 64-bit double.\");\n"
        << "static_assert(std::numeric_limits<double>::is_iec559,\n"
        << "              \"Requires IEEE-754 double.\");\n\n"
        << "int main() {\n"
        << "    const int num_rollers = " << params.num_rollers << ";\n"
        << "    const std::uint64_t num_ticks = " << params.num_ticks
        << "ULL;\n"
        << "    const double coupling = 0.3;\n"
        << "    const double pi_approx = 3.14159;\n"
        << "    const std::vector<double> roller_speeds = {";

    out << std::setprecision(std::numeric_limits<double>::max_digits10);
    for (int i = 0; i < params.num_rollers; ++i) {
        if (i != 0) {
            out << ", ";
        }
        out << params.roller_speeds[static_cast<std::size_t>(i)];
    }

    out << "};\n"
        << "    std::vector<double> positions("
        << "static_cast<std::size_t>(num_rollers), 0.0);\n"
        << "    std::vector<double> old_positions("
        << "static_cast<std::size_t>(num_rollers), 0.0);\n"
        << "    const std::uint64_t report_every = num_ticks / 10ULL;\n"
        << "    std::cout << \"Calculating...\\n\";\n"
        << "    for (std::uint64_t tick = 0; tick < num_ticks; ++tick) {\n"
        << "        for (int i = 0; i < num_rollers; ++i) {\n"
        << "            old_positions[static_cast<std::size_t>(i)] =\n"
        << "                positions[static_cast<std::size_t>(i)];\n"
        << "        }\n"
        << "        for (int i = 0; i < num_rollers; ++i) {\n"
        << "            const int next = (i + 1) % num_rollers;\n"
        << "            double position =\n"
        << "                old_positions[static_cast<std::size_t>(i)] +\n"
        << "                roller_speeds[static_cast<std::size_t>(i)];\n"
        << "            position += coupling * std::sin(\n"
        << "                old_positions[static_cast<std::size_t>(next)] *\n"
        << "                pi_approx);\n"
        << "            positions[static_cast<std::size_t>(i)] =\n"
        << "                std::round(position * 100.0) / 100.0;\n"
        << "        }\n"
        << "        if (report_every != 0 &&\n"
        << "            ((tick + 1ULL) % report_every == 0ULL ||\n"
        << "             tick + 1ULL == num_ticks)) {\n"
        << "            const std::uint64_t percent =\n"
        << "                ((tick + 1ULL) * 100ULL) / num_ticks;\n"
        << "            std::cout << \"Progress: \" << percent << \"%\\n\";\n"
        << "        }\n"
        << "    }\n"
        << "    std::uint64_t hash = 14695981039346656037ULL;\n"
        << "    std::string password;\n"
        << "    password.reserve(static_cast<std::size_t>(num_rollers));\n"
        << "    for (int char_idx = 0; char_idx < num_rollers; ++char_idx) {\n"
        << "        std::uint64_t position_bits = 0;\n"
        << "        std::memcpy(&position_bits,\n"
        << "                    &positions[static_cast<std::size_t>(char_idx)],\n"
        << "                    sizeof(position_bits));\n"
        << "        hash ^= position_bits;\n"
        << "        hash *= 1099511628211ULL;\n"
        << "        for (int j = 0; j < num_rollers; ++j) {\n"
        << "            if (j != char_idx) {\n"
        << "                std::uint64_t other_bits = 0;\n"
        << "                std::memcpy(&other_bits,\n"
        << "                            &positions[static_cast<std::size_t>(j)],\n"
        << "                            sizeof(other_bits));\n"
        << "                hash ^= other_bits;\n"
        << "                hash *= 1099511628211ULL;\n"
        << "            }\n"
        << "        }\n"
        << "        hash ^= static_cast<std::uint64_t>(char_idx) *\n"
        << "                0x9E3779B97F4A7C15ULL;\n"
        << "        hash *= 1099511628211ULL;\n"
        << "        const int ascii_value = 33 +\n"
        << "            static_cast<int>(hash % 94ULL);\n"
        << "        password.push_back(static_cast<char>(ascii_value));\n"
        << "    }\n"
        << R"(    std::cout << "PASSWORD: " << password << '\n';
)"
        << "    return 0;\n"
        << "}\n";

    out.close();
    return static_cast<bool>(out);
}

bool parse_u64(const std::string& text, std::uint64_t& value) {
    if (text.empty() || text[0] == '-') {
        return false;
    }

    errno = 0;
    char* end = 0;
    const unsigned long long parsed =
        std::strtoull(text.c_str(), &end, 10);
    if (errno == ERANGE || end == text.c_str() || *end != '\0' ||
        parsed > std::numeric_limits<std::uint64_t>::max()) {
        return false;
    }

    value = static_cast<std::uint64_t>(parsed);
    return true;
}

bool parse_roller_count(const std::string& text, int& value) {
    std::uint64_t parsed = 0;
    if (!parse_u64(text, parsed) || parsed >
            static_cast<std::uint64_t>(std::numeric_limits<int>::max())) {
        return false;
    }
    value = static_cast<int>(parsed);
    return true;
}

bool parse_seed(const std::string& text, std::uint32_t& seed) {
    std::uint64_t parsed = 0;
    if (!parse_u64(text, parsed) || parsed >
            static_cast<std::uint64_t>(std::numeric_limits<std::uint32_t>::max())) {
        return false;
    }
    seed = static_cast<std::uint32_t>(parsed);
    return true;
}

void print_usage(const char* program) {
    std::cout
        << "Usage:\n"
        << "  " << program << "                 Interactive generation\n"
        << "  " << program << " --emit R T [SEED]\n"
        << "      Write standalone source only (does not calculate password)\n"
        << "  " << program << " --generate R T [SEED]\n"
        << "      Write source and calculate password once\n"
        << "  " << program << " --self-test\n"
        << "\nR: rollers (5-20); T: ticks (1000-100000000000000).\n"
        << "The optional 32-bit seed makes parameter generation reproducible.\n";
}

bool run_self_test() {
    const std::vector<double> speeds = {0.17, 0.29, 0.43, 0.61, 0.83};
    const std::string first = calculate_password(5, 10000ULL, speeds);
    const std::string second = calculate_password(5, 10000ULL, speeds);

    if (first.size() != 5 || first != second) {
        std::cerr << "Self-test failed: non-deterministic result or bad length.\n";
        return false;
    }

    for (std::size_t i = 0; i < first.size(); ++i) {
        const unsigned char ch = static_cast<unsigned char>(first[i]);
        if (ch < 33U || ch > 126U) {
            std::cerr << "Self-test failed: output outside printable range.\n";
            return false;
        }
    }

    std::cout << "Self-test passed. Password length=" << first.size()
              << ", printable ASCII, deterministic for fixed parameters.\n";
    return true;
}

void print_estimate(const GeneratorParams& params) {
    const long double updates =
        static_cast<long double>(params.num_ticks) *
        static_cast<long double>(params.num_rollers);
    const long double seconds =
        updates / static_cast<long double>(kEstimatedUpdatesPerSecond);

    std::cout << "Estimated roller updates: " << std::fixed
              << std::setprecision(0) << updates << "\n";
    std::cout << "Estimated time (rough benchmark): ";
    if (seconds < 60.0L) {
        std::cout << std::setprecision(1) << seconds << " seconds\n";
    } else if (seconds < 3600.0L) {
        std::cout << std::setprecision(1) << seconds / 60.0L << " minutes\n";
    } else if (seconds < 86400.0L) {
        std::cout << std::setprecision(1) << seconds / 3600.0L << " hours\n";
    } else if (seconds < 31536000.0L) {
        std::cout << std::setprecision(1) << seconds / 86400.0L << " days\n";
    } else {
        std::cout << std::setprecision(2) << seconds / 31536000.0L
                  << " years\n";
    }
    std::cout << std::defaultfloat;
}

int generate(const GeneratorParams& params, bool calculate_now) {
    if (!valid_params(params)) {
        std::cerr << "Error: invalid roller/tick parameters.\n";
        return 1;
    }

    const std::string source_filename = "temporal_password_exe.cpp";
    if (!write_standalone_source(params, source_filename)) {
        std::cerr << "Error: could not write " << source_filename << ".\n";
        return 1;
    }

    std::cout << "Generated standalone source: " << source_filename << "\n";
    print_estimate(params);

    if (calculate_now) {
        std::cout << "Calculating password in this process...\n";
        const std::string password = calculate_password(
            params.num_rollers, params.num_ticks, params.roller_speeds);
        std::cout << "PASSWORD: " << password << "\n";
    } else {
        std::cout << "Source-only mode: password was not calculated.\n";
    }

    std::cout << "To build the generated program, run this manually with a C++11 compiler:\n"
              << "  c++ -std=c++11 -O2 temporal_password_exe.cpp -o temporal_password_exe\n";
    return 0;
}

}  // namespace temporal_password

int main(int argc, char* argv[]) {
    using namespace temporal_password;

    if (argc == 2 && std::string(argv[1]) == "--help") {
        print_usage(argv[0]);
        return 0;
    }
    if (argc == 2 && std::string(argv[1]) == "--self-test") {
        return run_self_test() ? 0 : 1;
    }

    bool calculate_now = true;
    int num_rollers = 0;
    std::uint64_t num_ticks = 0;
    std::uint32_t seed = 0;
    bool seeded = false;

    if (argc >= 2) {
        const std::string mode(argv[1]);
        if (mode != "--emit" && mode != "--generate") {
            print_usage(argv[0]);
            return 2;
        }
        if (argc < 4 || argc > 5) {
            print_usage(argv[0]);
            return 2;
        }

        if (!parse_roller_count(argv[2], num_rollers) ||
            !parse_u64(argv[3], num_ticks)) {
            std::cerr << "Error: R and T must be valid non-negative integers.\n";
            return 2;
        }
        if (argc == 5) {
            if (!parse_seed(argv[4], seed)) {
                std::cerr << "Error: SEED must be a 32-bit unsigned integer.\n";
                return 2;
            }
            seeded = true;
        }
        calculate_now = (mode == "--generate");
    } else {
        std::string input;
        std::cout << "Enter number of rollers (5-20): ";
        if (!(std::cin >> input) || !parse_roller_count(input, num_rollers)) {
            std::cerr << "Error: invalid roller count.\n";
            return 2;
        }

        std::cout << "Enter number of ticks (1000-100000000000000): ";
        if (!(std::cin >> input) || !parse_u64(input, num_ticks)) {
            std::cerr << "Error: invalid tick count.\n";
            return 2;
        }
    }

    if (num_rollers < kMinRollers || num_rollers > kMaxRollers) {
        std::cerr << "Error: number of rollers must be between "
                  << kMinRollers << " and " << kMaxRollers << ".\n";
        return 2;
    }
    if (num_ticks < kMinTicks || num_ticks > kMaxTicks) {
        std::cerr << "Error: number of ticks must be between "
                  << kMinTicks << " and " << kMaxTicks << ".\n";
        return 2;
    }

    if (!seeded) {
        seed = make_random_seed();
    }

    GeneratorParams params;
    params.num_rollers = num_rollers;
    params.num_ticks = num_ticks;
    params.roller_speeds = generate_roller_speeds(num_rollers, seed);

    std::cout << "Parameters: rollers=" << params.num_rollers
              << ", ticks=" << params.num_ticks << "\n"
              << "Roller speeds: " << std::setprecision(
                     std::numeric_limits<double>::max_digits10);
    for (std::size_t i = 0; i < params.roller_speeds.size(); ++i) {
        if (i != 0) {
            std::cout << ", ";
        }
        std::cout << params.roller_speeds[i];
    }
    std::cout << "\n";
    if (seeded) {
        std::cout << "Reproducibility seed: " << seed << "\n";
    }

    return generate(params, calculate_now);
}
