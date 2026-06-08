/**
 * @file CamJob.cpp
 * @brief Implementation of the CamJob class for managing CAM job data.
 */

#include "CamJob.h"

#include <algorithm>

namespace MilCAD {

/**
 * @brief Register a tool controller with the job after validation.
 *
 * Validates that the controller has positive IDs, that the referenced tool
 * exists in the library, and that no duplicate controller ID is present.
 *
 * @param controller The tool controller to add.
 * @return true if successfully added; false on validation failure or duplicate.
 */
bool CamJob::addToolController(const ToolController &controller)
{
    // Reject controllers with invalid IDs
    if (controller.id <= 0 || controller.toolId <= 0)
        return false;

    // Ensure the referenced tool exists in the library
    if (!toolLibrary_.find(controller.toolId))
        return false;

    // Check for duplicate controller ID
    const auto duplicate = std::find_if(toolControllers_.begin(), toolControllers_.end(),
                                        [&](const ToolController &c) { return c.id == controller.id; });
    if (duplicate != toolControllers_.end())
        return false;

    toolControllers_.push_back(controller);
    return true;
}

/**
 * @brief Look up a tool controller by its unique ID.
 * @param id The controller ID to search for.
 * @return Pointer to the found controller, or nullptr if not found.
 */
const ToolController *CamJob::toolController(int id) const
{
    auto it = std::find_if(toolControllers_.begin(), toolControllers_.end(),
                           [id](const ToolController &c) { return c.id == id; });
    return (it == toolControllers_.end()) ? nullptr : &(*it);
}

/**
 * @brief Add a machining operation to the job with ownership transfer.
 *
 * Auto-assigns an ID if the operation's ID is <= 0. Validates that any
 * referenced tool controller exists before accepting the operation.
 *
 * @param operation Unique pointer to the operation (ownership transferred).
 * @return true if added; false if null or tool controller reference is invalid.
 */
bool CamJob::addOperation(std::unique_ptr<Operation> operation)
{
    if (!operation)
        return false;

    // Auto-assign an ID if not explicitly set
    if (operation->id() <= 0)
        operation->setId(static_cast<int>(operations_.size()) + 1);

    // Validate tool controller reference if specified
    if (operation->toolControllerId() > 0 && !toolController(operation->toolControllerId()))
        return false;

    operations_.push_back(std::move(operation));
    return true;
}

} // namespace MilCAD
