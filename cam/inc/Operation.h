/**
 * @file Operation.h
 * @brief Abstract base class for CAM machining operations.
 *
 * Defines the Operation interface that all concrete machining strategies
 * (facing, pocketing, profiling, drilling, etc.) must implement.
 * Each operation carries an ID, a name, an optional tool controller binding,
 * coolant mode, and a method to generate the resulting Toolpath.
 */

#ifndef MILCAD_OPERATION_H
#define MILCAD_OPERATION_H

#include <memory>
#include <optional>
#include <string>

#include "CamGeometrySource.h"
#include "Toolpath.h"

namespace MilCAD {

/**
 * @enum CoolantMode
 * @brief Coolant activation mode for CNC operations.
 *
 * Maps to standard M-codes: Off=M9, Flood=M8, Mist=M7, Through=M8+M7.
 */
enum class CoolantMode {
    Off,     ///< No coolant (M9)
    Flood,   ///< Flood coolant (M8)
    Mist,    ///< Mist coolant (M7)
    Through  ///< Through-spindle coolant (M8 + spindle config)
};

/**
 * @class Operation
 * @brief Abstract base class for all CAM operations.
 *
 * Subclasses implement typeName() to identify the operation type and
 * generateToolpath() to produce the CNC motion sequence.
 */
class Operation
{
public:
    virtual ~Operation() = default;

    /// @brief Get the unique operation identifier.
    int id() const { return id_; }

    /// @brief Set the unique operation identifier.
    void setId(int id) { id_ = id; }

    /// @brief Get the ID of the associated tool controller.
    int toolControllerId() const { return toolControllerId_; }

    /// @brief Set the associated tool controller ID.
    void setToolControllerId(int id) { toolControllerId_ = id; }

    /// @brief Get the user-defined operation name.
    const std::string &name() const { return name_; }

    /// @brief Set the user-defined operation name.
    void setName(const std::string &name) { name_ = name; }

    /// @brief Get the coolant mode for this operation.
    CoolantMode coolantMode() const { return coolantMode_; }

    /// @brief Set the coolant mode (Off, Flood, Mist, Through).
    void setCoolantMode(CoolantMode mode) { coolantMode_ = mode; }

    /// @brief Set the CAD geometry source for this operation (sketch or part face).
    void setGeometrySource(CamGeometrySource source) { geometrySource_ = std::move(source); }

    /// @brief Get the CAD geometry source (if set).
    const std::optional<CamGeometrySource> &geometrySource() const { return geometrySource_; }

    /// @brief Check if the geometry source has changed since the last toolpath generation.
    bool isGeometryStale() const { return geometrySource_ && geometrySource_->isStale(); }

    /**
     * @brief Return a human-readable name for the operation type.
     * @return Type string such as "Facing", "Pocket", "Drill", etc.
     */
    virtual std::string typeName() const = 0;

    /**
     * @brief Generate the CNC toolpath for this operation.
     * @return A Toolpath containing all motion segments needed to execute the operation.
     */
    virtual Toolpath generateToolpath() const = 0;

private:
    int id_ = 0;               ///< Unique operation ID
    int toolControllerId_ = 0; ///< Bound tool controller ID (0 = none)
    std::string name_;         ///< User-assigned operation name
    CoolantMode coolantMode_ = CoolantMode::Off; ///< Coolant activation mode
    std::optional<CamGeometrySource> geometrySource_; ///< Link to CAD geometry source
};

/**
 * @class EmptyOperation
 * @brief A no-op operation that produces an empty toolpath.
 *
 * Useful as a placeholder or for testing purposes.
 */
class EmptyOperation : public Operation
{
public:
    std::string typeName() const override { return "Empty"; }
    Toolpath generateToolpath() const override { return Toolpath{}; }
};

} // namespace MilCAD

#endif // MILCAD_OPERATION_H
