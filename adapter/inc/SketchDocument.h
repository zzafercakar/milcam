#pragma once

#include "SketchEntity.h"

#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace MilCAD {

class SketchDocument
{
public:
    using EntityId = std::string;

    std::size_t entityCount() const { return entityIds_.size(); }

    const std::vector<EntityId> &entityIds() const { return entityIds_; }

    SketchEntity *entity(const EntityId &id)
    {
        const auto it = entities_.find(id);
        return it != entities_.end() ? it->second.get() : nullptr;
    }

    const SketchEntity *entity(const EntityId &id) const
    {
        const auto it = entities_.find(id);
        return it != entities_.end() ? it->second.get() : nullptr;
    }

    void addEntity(EntityId id, std::shared_ptr<SketchEntity> entity)
    {
        if (!entity) {
            return;
        }

        const auto [it, inserted] = entities_.insert_or_assign(id, std::move(entity));
        if (inserted) {
            entityIds_.push_back(it->first);
        }
    }

private:
    std::vector<EntityId> entityIds_;
    std::unordered_map<EntityId, std::shared_ptr<SketchEntity>> entities_;
};

} // namespace MilCAD
