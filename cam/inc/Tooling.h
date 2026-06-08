/**
 * @file Tooling.h
 * @brief CNC tool definitions, tool library management, tool controllers, and stock material.
 *
 * Provides the foundational data structures for the CAM module: cutting tool
 * geometry (Tool), a managed collection of tools (ToolLibrary), per-operation
 * spindle/feed parameters (ToolController), and workpiece stock dimensions (Stock).
 */

#ifndef MILCAD_TOOLING_H
#define MILCAD_TOOLING_H

#include <string>
#include <vector>

#include <gp_Pnt.hxx>

namespace MilCAD {

/**
 * @enum ToolType
 * @brief Enumerates the supported CNC cutting tool geometries.
 */
enum class ToolType {
    EndMill,  ///< Flat-end milling cutter
    BallMill, ///< Ball-nose milling cutter (hemispherical tip)
    Drill,    ///< Twist drill bit
    VBit,     ///< V-shaped engraving bit
    Laser     ///< Laser cutting head (non-contact)
};

/**
 * @struct Tool
 * @brief Describes the physical geometry of a single CNC cutting tool.
 */
struct Tool {
    int id = 0;                ///< Unique tool identifier (must be > 0 for valid tools)
    std::string name;          ///< Human-readable tool name
    ToolType type = ToolType::EndMill; ///< Geometric type of the tool
    double diameter = 0.0;     ///< Cutting diameter in mm
    double fluteLength = 0.0;  ///< Length of the cutting flutes in mm
    double overallLength = 0.0;///< Total tool length from holder to tip in mm
};

/**
 * @class ToolLibrary
 * @brief Manages an ordered collection of Tool instances with unique IDs.
 *
 * Provides add, remove, and lookup operations. Tool IDs must be positive
 * integers, and diameters must be positive for a tool to be accepted.
 */
class ToolLibrary
{
public:
    /**
     * @brief Add a tool to the library.
     * @param tool The tool to add. Must have id > 0 and diameter > 0.
     * @return true if the tool was added; false if validation failed or the ID already exists.
     */
    bool add(const Tool &tool);

    /**
     * @brief Remove a tool by its ID.
     * @param id The unique tool identifier to remove.
     * @return true if a tool with the given ID was found and removed; false otherwise.
     */
    bool remove(int id);

    /**
     * @brief Look up a tool by its ID.
     * @param id The unique tool identifier to search for.
     * @return Pointer to the matching Tool, or nullptr if not found.
     */
    const Tool *find(int id) const;

    /**
     * @brief Return a read-only reference to the full list of tools.
     * @return Const reference to the internal tool vector.
     */
    const std::vector<Tool> &all() const { return tools_; }

private:
    std::vector<Tool> tools_; ///< Ordered collection of tools
};

/**
 * @struct ToolController
 * @brief Binds a tool to spindle RPM and feed-rate parameters for a machining operation.
 */
struct ToolController {
    int id = 0;              ///< Unique controller identifier
    int toolId = 0;          ///< ID of the associated Tool in the ToolLibrary
    double spindleRpm = 0.0; ///< Spindle speed in revolutions per minute
    double feedCut = 0.0;    ///< Cutting feed rate in mm/min
    double feedPlunge = 0.0; ///< Plunge (Z-axis) feed rate in mm/min
    double feedRapid = 3000.0; ///< Rapid traverse rate in mm/min
};

/**
 * @struct Stock
 * @brief Defines the workpiece (raw material) bounding box and material type.
 */
struct Stock {
    gp_Pnt origin;           ///< Corner point (minimum X, Y, Z) of the stock block
    double sizeX = 0.0;      ///< Width of the stock in mm (X-axis extent)
    double sizeY = 0.0;      ///< Depth of the stock in mm (Y-axis extent)
    double sizeZ = 0.0;      ///< Height of the stock in mm (Z-axis extent)
    std::string material;    ///< Material description (e.g. "Aluminum 6061")
};

} // namespace MilCAD

#endif // MILCAD_TOOLING_H
