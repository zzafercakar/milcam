/**
 * @file GCodeParser.h
 * @brief Parser for reading G-code text into structured GCodeBlock objects.
 *
 * Parses multi-line G-code text, tokenizing each line into word fields
 * (N, G, X, Y, Z, R, Q, P, I, J, K, F, S, T, M). Supports optional error collection
 * for reporting invalid tokens encountered during parsing.
 */

#ifndef MILCAD_GCODE_PARSER_H
#define MILCAD_GCODE_PARSER_H

#include <string>
#include <vector>

#include "GCodeBlock.h"

namespace MilCAD {

/**
 * @class GCodeParser
 * @brief Static utility class that parses G-code text into GCodeBlock sequences.
 */
class GCodeParser
{
public:
    /**
     * @brief Parse G-code text into a vector of GCodeBlock objects.
     *
     * Each line of input produces one GCodeBlock. Tokens are space-separated
     * and consist of a letter code followed by a numeric value (e.g. "G1",
     * "X10.5"). Unrecognized letter codes are silently ignored; malformed
     * numeric values are reported via the errors parameter.
     *
     * @param text The G-code text to parse (may contain multiple lines).
     * @param errors Optional pointer to a vector that receives error messages
     *               for invalid tokens (e.g. "Line 3: invalid token 'Xabc'").
     * @return Vector of GCodeBlocks, one per input line.
     */
    static std::vector<GCodeBlock> parse(const std::string &text, std::vector<std::string> *errors = nullptr);
};

} // namespace MilCAD

#endif // MILCAD_GCODE_PARSER_H
