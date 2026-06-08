/**
 * @file CodesysStExporters.h
 * @brief Strategy interfaces and defaults for CODESYS Structured Text exports.
 */

#ifndef MILCAD_CODESYS_ST_EXPORTERS_H
#define MILCAD_CODESYS_ST_EXPORTERS_H

#include <string>
#include <vector>

#include "GCodeBlock.h"
#include "Toolpath.h"

namespace MilCAD {

class ICodesysRefArrayExporter
{
public:
    virtual ~ICodesysRefArrayExporter() = default;
    virtual std::string exportArray(const std::vector<GCodeBlock> &blocks,
                                    const std::string &arrayName) const = 0;
};

class ICodesysOutqueueExporter
{
public:
    virtual ~ICodesysOutqueueExporter() = default;
    virtual std::string exportQueue(const Toolpath &path,
                                    const std::string &arrayName) const = 0;
};

class SmcCncRefExporter : public ICodesysRefArrayExporter
{
public:
    std::string exportArray(const std::vector<GCodeBlock> &blocks,
                            const std::string &arrayName) const override;
};

class SmcOutqueueExporter : public ICodesysOutqueueExporter
{
public:
    std::string exportQueue(const Toolpath &path,
                            const std::string &arrayName) const override;
};

} // namespace MilCAD

#endif // MILCAD_CODESYS_ST_EXPORTERS_H
