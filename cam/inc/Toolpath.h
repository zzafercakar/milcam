/**
 * @file Toolpath.h
 * @brief Toolpath segment and toolpath container for CNC motion sequences.
 *
 * Defines the low-level motion primitives (rapid, linear, arc, plunge) that
 * represent CNC tool movement, and the Toolpath class that aggregates an
 * ordered sequence of these segments with bounding-box and timing utilities.
 */

#ifndef MILCAD_TOOLPATH_H
#define MILCAD_TOOLPATH_H

#include <vector>

#include <gp_Pnt.hxx>

namespace MilCAD {

/**
 * @enum ToolpathSegmentType
 * @brief Classifies the type of CNC motion for a single toolpath segment.
 */
enum class ToolpathSegmentType {
    Rapid,   ///< G0 rapid traverse (non-cutting, maximum speed)
    Linear,  ///< G1 linear interpolation (cutting move at specified feed)
    CWArc,   ///< G2 clockwise circular interpolation
    CCWArc,  ///< G3 counter-clockwise circular interpolation
    Plunge   ///< Vertical downward plunge move (treated as G1 in output)
};

/**
 * @struct ToolpathSegment
 * @brief A single CNC motion primitive with start/end points and optional arc center.
 */
struct ToolpathSegment {
    ToolpathSegmentType type = ToolpathSegmentType::Rapid; ///< Motion type
    gp_Pnt start;    ///< Starting position of the segment
    gp_Pnt end;      ///< Ending position of the segment
    gp_Pnt center;   ///< Arc center point (only meaningful for CWArc / CCWArc)
    double feedRate = 0.0; ///< Feed rate in mm/min (0 for rapid moves)
    double dwellSeconds = 0.0; ///< Optional dwell at segment end, in seconds

    /**
     * @brief Check whether this segment is a circular arc.
     * @return true if the segment type is CWArc or CCWArc.
     */
    bool isArc() const;

    /**
     * @brief Compute the geometric length of this segment.
     * @return Distance in mm. For arcs, computes the arc length from the
     *         angle between start and end vectors around center.
     */
    double length() const;
};

/**
 * @class Toolpath
 * @brief Ordered collection of ToolpathSegment instances forming a complete CNC tool motion.
 *
 * Provides segment storage, aggregate length/time calculations, and axis-aligned
 * bounding box queries over all contained segments.
 */
class Toolpath
{
public:
    /**
     * @brief Append a segment to the toolpath.
     * @param segment The toolpath segment to add.
     */
    void add(const ToolpathSegment &segment);

    /**
     * @brief Remove all segments from the toolpath.
     */
    void clear();

    /**
     * @brief Get a read-only reference to the segment list.
     * @return Const reference to the internal vector of segments.
     */
    const std::vector<ToolpathSegment> &segments() const { return segments_; }

    /**
     * @brief Check whether the toolpath contains any segments.
     * @return true if empty.
     */
    bool empty() const { return segments_.empty(); }

    /**
     * @brief Get the number of segments.
     * @return Segment count.
     */
    std::size_t size() const { return segments_.size(); }

    /**
     * @brief Compute the total geometric length of all segments.
     * @return Sum of individual segment lengths in mm.
     */
    double totalLength() const;

    /**
     * @brief Estimate total machining time based on feed rates.
     * @param rapidFeedRate The feed rate to assume for rapid (G0) moves, in mm/min.
     * @return Estimated time in minutes.
     */
    double estimatedTimeMinutes(double rapidFeedRate) const;

    /**
     * @brief Compute the minimum bounding point (min X, Y, Z) across all segments.
     * @return The minimum corner of the axis-aligned bounding box, or origin if empty.
     */
    gp_Pnt minPoint() const;

    /**
     * @brief Compute the maximum bounding point (max X, Y, Z) across all segments.
     * @return The maximum corner of the axis-aligned bounding box, or origin if empty.
     */
    gp_Pnt maxPoint() const;

private:
    std::vector<ToolpathSegment> segments_; ///< Ordered list of motion segments
};

} // namespace MilCAD

#endif // MILCAD_TOOLPATH_H
