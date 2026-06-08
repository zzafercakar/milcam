/**
 * @file Tooling.cpp
 * @brief CNC tool library management — add, remove, and look up cutting tools.
 *
 * Provides a simple in-memory collection of Tool definitions (id, diameter,
 * type, etc.) with uniqueness enforced by tool ID. Used by CAM operations
 * to select the appropriate cutting tool for toolpath generation.
 */

#include "Tooling.h"

#include <algorithm>

namespace MilCAD {

/**
 * @brief Add a new tool to the library.
 * @param tool  Tool definition to add (must have id > 0 and diameter > 0).
 * @return true if added successfully, false if the tool is invalid or the ID already exists.
 */
bool ToolLibrary::add(const Tool &tool)
{
    if (tool.id <= 0 || tool.diameter <= 0.0)
        return false;
    if (find(tool.id))
        return false;

    tools_.push_back(tool);
    return true;
}

/**
 * @brief Remove a tool from the library by its ID.
 * @param id  The tool ID to remove.
 * @return true if a tool was found and removed, false if not found.
 */
bool ToolLibrary::remove(int id)
{
    auto it = std::remove_if(tools_.begin(), tools_.end(),
                             [id](const Tool &t) { return t.id == id; });
    if (it == tools_.end())
        return false;

    tools_.erase(it, tools_.end());
    return true;
}

/**
 * @brief Find a tool in the library by its ID.
 * @param id  The tool ID to search for.
 * @return Pointer to the Tool if found, nullptr otherwise.
 */
const Tool *ToolLibrary::find(int id) const
{
    auto it = std::find_if(tools_.begin(), tools_.end(),
                           [id](const Tool &t) { return t.id == id; });
    return (it == tools_.end()) ? nullptr : &(*it);
}

} // namespace MilCAD
