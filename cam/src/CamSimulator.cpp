/**
 * @file CamSimulator.cpp
 * @brief Implementation of the CamSimulator step-by-step toolpath playback.
 */

#include "CamSimulator.h"

#include <algorithm>
#include <cmath>

namespace MilCAD {

/**
 * @brief Load a toolpath and reset simulation state.
 * @param toolpath The toolpath to simulate.
 */
void CamSimulator::loadToolpath(const Toolpath &toolpath)
{
    source_ = toolpath;
    totalSteps_ = static_cast<int>(source_.size());
    currentStep_ = 0;
    running_ = false;
}

/**
 * @brief Reset the step counter to zero and stop playback.
 */
void CamSimulator::reset()
{
    currentStep_ = 0;
    running_ = false;
}

/**
 * @brief Advance the simulation by one segment.
 * @return true if a step was taken; false if already at the end.
 */
bool CamSimulator::stepForward()
{
    if (currentStep_ >= totalSteps_) {
        running_ = false;
        return false;
    }

    ++currentStep_;

    // Auto-stop when the last segment is reached
    if (currentStep_ >= totalSteps_)
        running_ = false;
    return true;
}

/**
 * @brief Set the playback speed multiplier, clamped to [0.1, 100.0].
 * @param speed The desired speed multiplier.
 */
void CamSimulator::setSpeedMultiplier(double speed)
{
    speedMultiplier_ = std::clamp(speed, 0.1, 100.0);
}

/**
 * @brief Compute the suggested timer interval for continuous playback.
 *
 * Uses a base interval of 140ms divided by the speed multiplier,
 * clamped to [10, 1000] ms.
 *
 * @return Suggested interval in milliseconds.
 */
int CamSimulator::suggestedIntervalMs() const
{
    const double baseMs = 140.0;
    const int value = static_cast<int>(std::round(baseMs / speedMultiplier_));
    return std::clamp(value, 10, 1000);
}

/**
 * @brief Compute the simulation progress as a fraction in [0.0, 1.0].
 * @return Progress fraction, or 0.0 if no steps are loaded.
 */
double CamSimulator::progress() const
{
    if (totalSteps_ <= 0)
        return 0.0;
    return static_cast<double>(currentStep_) / static_cast<double>(totalSteps_);
}

/**
 * @brief Get the current tool position based on simulation state.
 * @return End point of the last completed segment, or origin if empty.
 */
gp_Pnt CamSimulator::toolPosition() const
{
    if (source_.segments().empty())
        return gp_Pnt(0, 0, 0);

    // Before any steps, return the first segment's start position
    if (currentStep_ <= 0)
        return source_.segments().front().start;

    // Return the end point of the most recently completed segment
    const int idx = std::clamp(currentStep_ - 1, 0, totalSteps_ - 1);
    return source_.segments()[static_cast<std::size_t>(idx)].end;
}

/**
 * @brief Build a partial toolpath containing only the traversed segments.
 * @return A new Toolpath with segments [0, currentStep).
 */
Toolpath CamSimulator::visibleToolpath() const
{
    Toolpath out;
    const int count = std::clamp(currentStep_, 0, totalSteps_);
    for (int i = 0; i < count; ++i)
        out.add(source_.segments()[static_cast<std::size_t>(i)]);
    return out;
}

} // namespace MilCAD
