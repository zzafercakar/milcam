#pragma once

#include <TopoDS_Edge.hxx>
#include <TopoDS_Wire.hxx>

namespace MilCAD {

enum class SketchEntityType {
    Line,
    Arc,
    Circle,
    Rectangle,
    Ellipse,
    Spline,
    Polygon,
    Unknown
};

class SketchEntity
{
public:
    virtual ~SketchEntity() = default;

    virtual bool isConstruction() const { return false; }
    virtual SketchEntityType type() const { return SketchEntityType::Unknown; }
    virtual TopoDS_Edge toEdge() const { return {}; }
    virtual TopoDS_Wire toWire() const { return {}; }
};

} // namespace MilCAD
