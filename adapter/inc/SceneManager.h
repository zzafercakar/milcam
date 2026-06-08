#pragma once

#include <string>
#include <unordered_map>

#include <Quantity_Color.hxx>
#include <TopoDS_Shape.hxx>

namespace MilCAD {

class SceneManager
{
public:
    bool addShape(const std::string &id, const TopoDS_Shape &shape, const Quantity_Color &, bool)
    {
        if (id.empty() || shape.IsNull()) {
            return false;
        }

        shapes_.insert_or_assign(id, shape);
        return true;
    }

    void removeShape(const std::string &id)
    {
        shapes_.erase(id);
    }

    void setShapeTransparency(const std::string &, double)
    {
    }

private:
    std::unordered_map<std::string, TopoDS_Shape> shapes_;
};

} // namespace MilCAD
