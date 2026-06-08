/**
 * @file GCodeBlock.cpp
 * @brief Serialization of a single G-code block (line) to its textual representation.
 *
 * Each GCodeBlock holds optional fields for all standard G-code address letters
 * (N, G, X, Y, Z, R, Q, P, I, J, K, F, S, T, M). The toString() method emits only the
 * fields that are set, formatted with 3-decimal-place precision for real values.
 */

#include "GCodeBlock.h"

#include <iomanip>
#include <sstream>

namespace MilCAD {

/**
 * @brief Serialize this G-code block into a single-line text string.
 * @return Formatted G-code line (e.g. "N10 G1 X12.500 Y0.000 Z-3.000 F200.000").
 *
 * Only fields that have a value are emitted. Integer fields (N, G, T, M) are
 * written without decimals; real fields (X, Y, Z, R, Q, P, I, J, K, F, S) use 3 decimal
 * places. A trailing space, if any, is trimmed before returning.
 */
std::string GCodeBlock::toString() const
{
    std::ostringstream ss;
    ss << std::fixed << std::setprecision(3);

    auto emitInt = [&](const char code, const std::optional<int> &v) {
        if (v) ss << code << *v << ' ';
    };
    auto emitDouble = [&](const char code, const std::optional<double> &v) {
        if (v) ss << code << *v << ' ';
    };

    emitInt('N', n);
    emitInt('G', g);
    emitDouble('X', x);
    emitDouble('Y', y);
    emitDouble('Z', z);
    emitDouble('R', r);
    emitDouble('Q', q);
    emitDouble('P', p);
    emitDouble('I', i);
    emitDouble('J', j);
    emitDouble('K', k);
    emitDouble('F', f);
    emitDouble('S', s);
    emitInt('T', t);
    emitInt('M', m);

    auto out = ss.str();
    if (!out.empty() && out.back() == ' ')
        out.pop_back();
    return out;
}

} // namespace MilCAD
