/**
 * @file Toolpath.cpp
 * @brief Core toolpath data structure — segment operations and path-level queries.
 *
 * Implements the ToolpathSegment and Toolpath classes that form the backbone of
 * the CAM module. A Toolpath is an ordered sequence of ToolpathSegments, each
 * representing a single machine motion (rapid, linear, plunge, or arc). This
 * file provides segment length calculation (including arc length via subtended
 * angle), path-level aggregate queries (total length, estimated machining time),
 * and bounding box computation.
 */

#include "Toolpath.h"

#include <algorithm>
#include <cmath>
#include <gp_Vec.hxx>

namespace MilCAD {

/**
 * @brief Check whether this segment is an arc (CW or CCW).
 * @return true if the segment type is CWArc or CCWArc.
 */
bool ToolpathSegment::isArc() const
{
    return type == ToolpathSegmentType::CWArc || type == ToolpathSegmentType::CCWArc;
}

/**
 * @brief Calculate the geometric length of this segment.
 * @return Length in mm. For linear/rapid/plunge segments, this is the Euclidean
 *         distance. For arcs, it is the arc length (radius * subtended angle).
 */
double ToolpathSegment::length() const
{
    if (!isArc())
        return start.Distance(end);

    const double radius = center.Distance(start);
    if (radius <= 1e-9)
        return start.Distance(end);

    const gp_Vec v0(center, start);
    const gp_Vec v1(center, end);
    if (v0.Magnitude() <= 1e-9 || v1.Magnitude() <= 1e-9)
        return start.Distance(end);

    double ang = v0.Angle(v1);
    if (type == ToolpathSegmentType::CWArc)
        ang = 2.0 * M_PI - ang;

    return radius * ang;
}

/**
 * @brief Append a segment to the end of the toolpath.
 * @param segment  The toolpath segment to add.
 */
void Toolpath::add(const ToolpathSegment &segment)
{
    segments_.push_back(segment);
}

/**
 * @brief Remove all segments from the toolpath.
 */
void Toolpath::clear()
{
    segments_.clear();
}

/**
 * @brief Calculate the total length of all segments in the toolpath.
 * @return Sum of all segment lengths in mm.
 */
double Toolpath::totalLength() const
{
    double sum = 0.0;
    for (const auto &s : segments_)
        sum += s.length();
    return sum;
}

/**
 * @brief Estimate machining time based on segment lengths and feed rates.
 * @param rapidFeedRate  Feed rate to assume for rapid (G0) moves (mm/min).
 * @return Estimated total machining time in minutes.
 *
 * For cutting segments, the segment's own feedRate is used. For rapid moves,
 * the provided rapidFeedRate is used. Segments with zero or near-zero feed
 * rates are skipped to avoid division by zero. Any per-segment dwellSeconds
 * is added as extra time.
 */
double Toolpath::estimatedTimeMinutes(double rapidFeedRate) const
{
    double minutes = 0.0;
    for (const auto &s : segments_) {
        const double feed = (s.type == ToolpathSegmentType::Rapid)
            ? rapidFeedRate
            : s.feedRate;
        if (feed > 1e-9)
            minutes += s.length() / feed;
        if (s.dwellSeconds > 0.0)
            minutes += (s.dwellSeconds / 60.0);
    }
    return minutes;
}

/**
 * @brief Compute the minimum bounding-box corner of the entire toolpath.
 * @return gp_Pnt with the smallest X, Y, and Z values across all segment endpoints.
 */
gp_Pnt Toolpath::minPoint() const
{
    if (segments_.empty())
        return gp_Pnt(0, 0, 0);

    double minX = segments_.front().start.X();
    double minY = segments_.front().start.Y();
    double minZ = segments_.front().start.Z();

    const auto include = [&](const gp_Pnt &p) {
        minX = std::min(minX, p.X());
        minY = std::min(minY, p.Y());
        minZ = std::min(minZ, p.Z());
    };

    for (const auto &s : segments_) {
        include(s.start);
        include(s.end);
        if (s.isArc())
            include(s.center);
    }

    return gp_Pnt(minX, minY, minZ);
}

/**
 * @brief Compute the maximum bounding-box corner of the entire toolpath.
 * @return gp_Pnt with the largest X, Y, and Z values across all segment endpoints.
 */
gp_Pnt Toolpath::maxPoint() const
{
    if (segments_.empty())
        return gp_Pnt(0, 0, 0);

    double maxX = segments_.front().start.X();
    double maxY = segments_.front().start.Y();
    double maxZ = segments_.front().start.Z();

    const auto include = [&](const gp_Pnt &p) {
        maxX = std::max(maxX, p.X());
        maxY = std::max(maxY, p.Y());
        maxZ = std::max(maxZ, p.Z());
    };

    for (const auto &s : segments_) {
        include(s.start);
        include(s.end);
        if (s.isArc())
            include(s.center);
    }

    return gp_Pnt(maxX, maxY, maxZ);
}

} // namespace MilCAD
