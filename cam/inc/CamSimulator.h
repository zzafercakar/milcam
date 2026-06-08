/**
 * @file CamSimulator.h
 * @brief Step-by-step toolpath playback simulator for visualization.
 *
 * CamSimulator loads a Toolpath and allows stepping through it one segment
 * at a time, tracking the current tool position and progress. It supports
 * adjustable playback speed and provides the visible (already traversed)
 * portion of the toolpath for rendering.
 */

#ifndef MILCAD_CAM_SIMULATOR_H
#define MILCAD_CAM_SIMULATOR_H

#include "Toolpath.h"

namespace MilCAD {

/**
 * @class CamSimulator
 * @brief Simulates CNC toolpath execution by stepping through segments sequentially.
 *
 * The simulator maintains a current step index and provides tool position,
 * progress percentage, and the partial toolpath for visual feedback. Speed
 * can be adjusted via a multiplier that affects the suggested timer interval.
 */
class CamSimulator
{
public:
    /**
     * @brief Load a toolpath for simulation.
     * @param toolpath The toolpath to simulate. Resets step counter and stops playback.
     */
    void loadToolpath(const Toolpath &toolpath);

    /**
     * @brief Reset the simulation to the beginning without clearing the loaded toolpath.
     */
    void reset();

    /**
     * @brief Advance the simulation by one segment.
     * @return true if the step was successful; false if already at the end.
     *         Automatically stops playback when the last segment is reached.
     */
    bool stepForward();

    /**
     * @brief Set the playback running state.
     * @param running true to start continuous playback; false to pause.
     */
    void setRunning(bool running) { running_ = running; }

    /// @brief Check whether continuous playback is active.
    bool running() const { return running_; }

    /**
     * @brief Set the playback speed multiplier.
     * @param speed Multiplier clamped to [0.1, 100.0]. Higher values = faster playback.
     */
    void setSpeedMultiplier(double speed);

    /// @brief Get the current playback speed multiplier.
    double speedMultiplier() const { return speedMultiplier_; }

    /**
     * @brief Compute the suggested timer interval for continuous playback.
     * @return Interval in milliseconds, clamped to [10, 1000], inversely proportional
     *         to the speed multiplier.
     */
    int suggestedIntervalMs() const;

    /// @brief Get the current step index (0-based, 0 means not yet started).
    int currentStep() const { return currentStep_; }

    /// @brief Get the total number of steps (segments) in the loaded toolpath.
    int totalSteps() const { return totalSteps_; }

    /**
     * @brief Compute the simulation progress as a fraction.
     * @return Value in [0.0, 1.0] representing the fraction of segments traversed.
     */
    double progress() const;

    /**
     * @brief Get the current tool position based on the simulation state.
     * @return The end point of the most recently completed segment, or the
     *         first segment's start point if no steps have been taken.
     */
    gp_Pnt toolPosition() const;

    /**
     * @brief Build a partial toolpath containing only the segments traversed so far.
     * @return A new Toolpath with segments [0, currentStep).
     */
    Toolpath visibleToolpath() const;

private:
    Toolpath source_;             ///< The full toolpath being simulated
    int currentStep_ = 0;        ///< Number of segments completed
    int totalSteps_ = 0;         ///< Total segment count
    bool running_ = false;        ///< Whether continuous playback is active
    double speedMultiplier_ = 1.0;///< Playback speed factor
};

} // namespace MilCAD

#endif // MILCAD_CAM_SIMULATOR_H
