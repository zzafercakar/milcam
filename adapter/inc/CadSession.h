#pragma once

/**
 * @file CadSession.h
 * @brief Top-level session managing FreeCAD application lifecycle.
 *
 * CadSession initializes the FreeCAD App framework (Base, App, Part, Sketcher,
 * PartDesign modules), manages document creation/loading, and provides the
 * entry point for all CAD operations. UI code should only interact through
 * this facade and never include FreeCAD headers directly.
 */

#include <memory>
#include <string>
#include <vector>

namespace CADNC {

class CadDocument;

class CadSession {
public:
    CadSession();
    ~CadSession();

    /// Initialize FreeCAD application framework (call once at startup)
    bool initialize(int argc, char** argv);

    /// Shutdown and cleanup
    void shutdown();

    /// True after successful initialize()
    bool isInitialized() const { return initialized_; }

    // ── Document management ─────────────────────────────────────────
    /// Create a new empty document
    std::shared_ptr<CadDocument> newDocument(const std::string& name = "Untitled");

    /// Close a document by name
    void closeDocument(const std::string& name);

    /// List all open document names
    std::vector<std::string> documentNames() const;

private:
    bool initialized_ = false;
    std::vector<std::shared_ptr<CadDocument>> documents_;
};

} // namespace CADNC
