/**
 * @file CamJob.h
 * @brief Top-level CAM job container that aggregates stock, tools, and operations.
 *
 * A CamJob represents a complete machining session: it holds the workpiece
 * stock definition, a work coordinate system (WCS), a tool library, tool
 * controllers (binding tools to spindle/feed settings), and an ordered list
 * of machining operations that produce toolpaths.
 */

#ifndef MILCAD_CAM_JOB_H
#define MILCAD_CAM_JOB_H

#include <memory>
#include <vector>

#include <gp_Ax3.hxx>

#include "Operation.h"
#include "Tooling.h"

namespace MilCAD {

/**
 * @class CamJob
 * @brief Aggregates all data needed for a CNC machining job.
 *
 * Manages ownership of Operation instances via unique_ptr, validates
 * tool controller references on insertion, and auto-assigns operation IDs
 * when not explicitly set.
 */
class CamJob
{
public:
    /// @brief Get the workpiece stock definition.
    const Stock &stock() const { return stock_; }

    /// @brief Set the workpiece stock definition.
    void setStock(const Stock &stock) { stock_ = stock; }

    /// @brief Get the work coordinate system (WCS).
    const gp_Ax3 &wcs() const { return wcs_; }

    /// @brief Set the work coordinate system (WCS).
    void setWcs(const gp_Ax3 &wcs) { wcs_ = wcs; }

    /// @brief Get a mutable reference to the tool library.
    ToolLibrary &toolLibrary() { return toolLibrary_; }

    /// @brief Get a read-only reference to the tool library.
    const ToolLibrary &toolLibrary() const { return toolLibrary_; }

    /**
     * @brief Register a tool controller with this job.
     * @param controller The controller to add. Must have id > 0, toolId > 0,
     *        and the referenced tool must exist in the tool library.
     * @return true if the controller was added; false if validation failed
     *         or a controller with the same ID already exists.
     */
    bool addToolController(const ToolController &controller);

    /**
     * @brief Look up a tool controller by its ID.
     * @param id The controller ID to search for.
     * @return Pointer to the matching ToolController, or nullptr if not found.
     */
    const ToolController *toolController(int id) const;

    /**
     * @brief Add a machining operation to the job.
     * @param operation Unique pointer to the operation. Ownership is transferred.
     *        If the operation's ID is <= 0, it is auto-assigned. If the operation
     *        references a tool controller, that controller must already exist.
     * @return true if the operation was added; false if null or validation failed.
     */
    bool addOperation(std::unique_ptr<Operation> operation);

    /// @brief Get the number of registered operations.
    std::size_t operationCount() const { return operations_.size(); }

    /// @brief Get a read-only reference to the list of operations.
    const std::vector<std::unique_ptr<Operation>> &operations() const { return operations_; }

private:
    Stock stock_;                                ///< Workpiece stock definition
    gp_Ax3 wcs_;                                 ///< Work coordinate system
    ToolLibrary toolLibrary_;                    ///< Available cutting tools
    std::vector<ToolController> toolControllers_;///< Registered tool controllers
    std::vector<std::unique_ptr<Operation>> operations_; ///< Ordered machining operations
};

} // namespace MilCAD

#endif // MILCAD_CAM_JOB_H
