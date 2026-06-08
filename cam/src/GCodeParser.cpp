/**
 * @file GCodeParser.cpp
 * @brief Parses G-code text into structured GCodeBlock objects.
 *
 * Reads a multi-line G-code string and tokenizes each line by whitespace.
 * Each token is expected to start with an address letter (G, X, Y, Z, R, Q, P, etc.)
 * followed by a numeric value. Unrecognized or malformed tokens are reported
 * via an optional error vector. One GCodeBlock is emitted per input line,
 * even if the line is empty.
 */

#include "GCodeParser.h"

#include <cstdlib>
#include <sstream>

namespace MilCAD {

/**
 * @brief Parse a G-code text string into a vector of GCodeBlock.
 * @param text  Multi-line G-code program text.
 * @param errors  Optional output vector for parse error messages (may be nullptr).
 * @return Vector of GCodeBlock, one per input line.
 *
 * Each whitespace-separated token is parsed by its leading letter code.
 * Integer fields (N, G, T, M) use strtol; real fields (X, Y, Z, R, Q, P, I, J, K, F, S)
 * use strtod. Tokens that fail to parse are reported in the errors vector.
 */
std::vector<GCodeBlock> GCodeParser::parse(const std::string &text, std::vector<std::string> *errors)
{
    std::vector<GCodeBlock> blocks;

    std::istringstream lines(text);
    std::string line;
    int lineNo = 0;

    while (std::getline(lines, line)) {
        ++lineNo;
        GCodeBlock b;

        std::istringstream tokens(line);
        std::string tok;
        while (tokens >> tok) {
            if (tok.empty())
                continue;

            const char code = tok[0];
            const std::string val = tok.substr(1);
            char *endPtr = nullptr;

            const auto parseInt = [&]() -> std::optional<int> {
                long v = std::strtol(val.c_str(), &endPtr, 10);
                if (endPtr == val.c_str() || *endPtr != '\0')
                    return std::nullopt;
                return static_cast<int>(v);
            };
            const auto parseDouble = [&]() -> std::optional<double> {
                double v = std::strtod(val.c_str(), &endPtr);
                if (endPtr == val.c_str() || *endPtr != '\0')
                    return std::nullopt;
                return v;
            };

            bool ok = true;
            switch (code) {
            case 'N': { auto v = parseInt(); if (v) b.n = *v; else ok = false; break; }
            case 'G': { auto v = parseInt(); if (v) b.g = *v; else ok = false; break; }
            case 'X': { auto v = parseDouble(); if (v) b.x = *v; else ok = false; break; }
            case 'Y': { auto v = parseDouble(); if (v) b.y = *v; else ok = false; break; }
            case 'Z': { auto v = parseDouble(); if (v) b.z = *v; else ok = false; break; }
            case 'R': { auto v = parseDouble(); if (v) b.r = *v; else ok = false; break; }
            case 'Q': { auto v = parseDouble(); if (v) b.q = *v; else ok = false; break; }
            case 'P': { auto v = parseDouble(); if (v) b.p = *v; else ok = false; break; }
            case 'I': { auto v = parseDouble(); if (v) b.i = *v; else ok = false; break; }
            case 'J': { auto v = parseDouble(); if (v) b.j = *v; else ok = false; break; }
            case 'K': { auto v = parseDouble(); if (v) b.k = *v; else ok = false; break; }
            case 'F': { auto v = parseDouble(); if (v) b.f = *v; else ok = false; break; }
            case 'S': { auto v = parseDouble(); if (v) b.s = *v; else ok = false; break; }
            case 'T': { auto v = parseInt(); if (v) b.t = *v; else ok = false; break; }
            case 'M': { auto v = parseInt(); if (v) b.m = *v; else ok = false; break; }
            default:
                break;
            }

            if (!ok && errors)
                errors->push_back("Line " + std::to_string(lineNo) + ": invalid token '" + tok + "'");
        }

        blocks.push_back(b);
    }

    return blocks;
}

} // namespace MilCAD
