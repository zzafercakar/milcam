/**
 * @file GCodeBlock.h
 * @brief Structured representation of a single G-code line (block).
 *
 * Each GCodeBlock holds optional word fields (N, G, X, Y, Z, R, Q, P, I, J, K, F, S, T, M)
 * that mirror the standard RS-274 / DIN 66025 G-code word format. Only fields
 * that are set (via std::optional) are emitted when serialized to text.
 */

#ifndef MILCAD_GCODE_BLOCK_H
#define MILCAD_GCODE_BLOCK_H

#include <optional>
#include <string>

namespace MilCAD {

/**
 * @struct GCodeBlock
 * @brief Represents one line of G-code with optional address fields.
 *
 * Fields use std::optional so that only explicitly set words appear in
 * the serialized output. This avoids emitting unnecessary coordinates
 * and keeps the output clean for post-processors.
 */
struct GCodeBlock {
    std::optional<int> n;      ///< N-word: line/sequence number
    std::optional<int> g;      ///< G-word: preparatory command (e.g. 0=rapid, 1=linear)
    std::optional<double> x;   ///< X-word: X-axis coordinate
    std::optional<double> y;   ///< Y-word: Y-axis coordinate
    std::optional<double> z;   ///< Z-word: Z-axis coordinate
    std::optional<double> r;   ///< R-word: retract / reference plane (cycles)
    std::optional<double> q;   ///< Q-word: peck depth increment (cycles)
    std::optional<double> p;   ///< P-word: dwell time in seconds (cycles)
    std::optional<double> i;   ///< I-word: arc center X offset (incremental from start)
    std::optional<double> j;   ///< J-word: arc center Y offset (incremental from start)
    std::optional<double> k;   ///< K-word: arc center Z offset (incremental from start)
    std::optional<double> f;   ///< F-word: feed rate in mm/min
    std::optional<double> s;   ///< S-word: spindle speed in RPM
    std::optional<int> t;      ///< T-word: tool number
    std::optional<int> m;      ///< M-word: miscellaneous function (e.g. 30=program end)

    /**
     * @brief Serialize this block to a single G-code text line.
     * @return A string like "N10 G1 X10.000 Y20.000 F1200.000" with only set fields.
     */
    std::string toString() const;
};

} // namespace MilCAD

#endif // MILCAD_GCODE_BLOCK_H
