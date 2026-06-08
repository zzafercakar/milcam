#include "NestFacade.h"

#include "NestJob.h"
#include "NestingEngine.h"

#include <set>

namespace CADNC {

NestFacade::NestFacade()
    : job_(std::make_unique<MilCAD::NestJob>())
{
}

NestFacade::~NestFacade() = default;

bool NestFacade::addPart(const std::string& id, double width, double height,
                         int quantity, bool allowRotation)
{
    MilCAD::NestPart part;
    part.id = id;
    part.bounds = {0, 0, width, height};
    part.quantity = quantity;
    part.allowRotation = allowRotation;
    part.allowMirror = false;
    return job_->addPart(part);
}

void NestFacade::clearParts()
{
    job_->clear();
}

int NestFacade::partCount() const
{
    return static_cast<int>(job_->parts().size());
}

void NestFacade::setSheet(const std::string& id, double width, double height)
{
    // Clear existing sheets and add new one
    job_->sheets().clear();
    MilCAD::NestSheet sheet;
    sheet.id = id;
    sheet.width = width;
    sheet.height = height;
    job_->addSheet(sheet);
}

void NestFacade::setPartGap(double gap)
{
    job_->params().partGap = gap;
}

void NestFacade::setEdgeGap(double gap)
{
    job_->params().edgeGap = gap;
}

void NestFacade::setRotationMode(int mode)
{
    switch (mode) {
    case 0: job_->params().rotationMode = MilCAD::NestRotationMode::None; break;
    case 1: job_->params().rotationMode = MilCAD::NestRotationMode::Quadrant; break;
    case 2: job_->params().rotationMode = MilCAD::NestRotationMode::Free; break;
    default: job_->params().rotationMode = MilCAD::NestRotationMode::Quadrant; break;
    }
}

void NestFacade::setOptimizationTime(double seconds)
{
    job_->params().optimizationSeconds = seconds;
}

NestResultInfo NestFacade::run(int algorithm)
{
    MilCAD::NestingEngine engine;
    auto alg = (algorithm == 0) ? MilCAD::NestingAlgorithm::BoundingBoxRows
                                : MilCAD::NestingAlgorithm::BottomLeftFill;

    auto result = engine.run(*job_, alg);
    job_->setResult(result);

    // Convert to UI-friendly format
    lastResult_ = {};
    lastResult_.totalPlaced = static_cast<int>(result.placements.size());
    lastResult_.totalUnplaced = static_cast<int>(result.unplacedPartIds.size());
    lastResult_.utilization = result.utilization();
    // Count unique sheet indices to determine sheets used
    {
        std::set<int> usedSheets;
        for (const auto& p : result.placements)
            usedSheets.insert(p.sheetIndex);
        lastResult_.sheetsUsed = static_cast<int>(usedSheets.size());
    }

    for (const auto& p : result.placements) {
        NestPlacementInfo pi;
        pi.partId = p.partId;
        pi.x = p.x;
        pi.y = p.y;
        pi.rotation = p.rotationDeg;
        pi.sheetIndex = p.sheetIndex;
        lastResult_.placements.push_back(std::move(pi));
    }

    lastResult_.unplacedIds = result.unplacedPartIds;
    return lastResult_;
}

NestResultInfo NestFacade::lastResult() const
{
    return lastResult_;
}

} // namespace CADNC
