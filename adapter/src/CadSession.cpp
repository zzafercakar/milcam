#include "CadSession.h"
#include "CadDocument.h"

#include <CXX/WrapPython.h>
#include <Base/Console.h>
#include <Base/Interpreter.h>
#include <App/Application.h>

// Module init functions — register with Python before Py_Initialize
extern "C" PyObject* PyInit_Part();
extern "C" PyObject* PyInit_Sketcher();
extern "C" PyObject* PyInit_Materials();
extern "C" PyObject* PyInit__PartDesign();

namespace CADNC {

CadSession::CadSession() = default;

CadSession::~CadSession() { shutdown(); }

bool CadSession::initialize(int argc, char** argv)
{
    if (initialized_) return true;

    try {
        // Register FreeCAD module Python entry points before Py_Initialize
        PyImport_AppendInittab("Part", PyInit_Part);
        PyImport_AppendInittab("Sketcher", PyInit_Sketcher);
        PyImport_AppendInittab("Materials", PyInit_Materials);
        PyImport_AppendInittab("_PartDesign", PyInit__PartDesign);

        // Full FreeCAD application init (Python, types, config, scripts)
        App::Application::init(argc, argv);

        // BUG-010 fix: explicitly load the FreeCAD C++ extension modules so
        // their PyMOD_INIT_FUNC bodies run — each one calls `::init()` on
        // every type the module owns (PartDesign::Body, PartDesign::Pad,
        // etc.), registering them with Base::Type. Without this, the string
        // factory `doc->addObject("PartDesign::Body", ...)` fails with
        // "type not registered" and features fall back to OCCT.
        //
        // We also alias `_PartDesign` as `PartDesign` in sys.modules so
        // `Type::getTypeIfDerivedFrom` succeeds when it does its implicit
        // `Interpreter().loadModule("PartDesign")` — FreeCAD normally ships
        // a Python wrapper package under that name; we don't, so we alias.
        try {
            Base::Interpreter().runString("import Part");
            Base::Interpreter().runString("import Sketcher");
            Base::Interpreter().runString("import Materials");
            Base::Interpreter().runString("import _PartDesign");
            Base::Interpreter().runString(
                "import sys; sys.modules['PartDesign'] = sys.modules['_PartDesign']");
        } catch (const Base::Exception& e) {
            fprintf(stderr, "CADNC: module import warning: %s\n", e.what());
            // Not fatal — sketch-only workflow still works via the OCCT fallback.
        }

        Base::Console().log("CADNC: FreeCAD backend initialized\n");
        initialized_ = true;
        return true;
    }
    catch (const Base::Exception& e) {
        fprintf(stderr, "CADNC: FreeCAD init failed: %s\n", e.what());
        return false;
    }
    catch (const std::exception& e) {
        fprintf(stderr, "CADNC: init failed: %s\n", e.what());
        return false;
    }
}

void CadSession::shutdown()
{
    if (!initialized_) return;

    documents_.clear();

    try {
        App::Application::destruct();
    } catch (...) {}

    initialized_ = false;
}

std::shared_ptr<CadDocument> CadSession::newDocument(const std::string& name)
{
    auto doc = std::make_shared<CadDocument>(name);
    documents_.push_back(doc);
    return doc;
}

void CadSession::closeDocument(const std::string& name)
{
    documents_.erase(
        std::remove_if(documents_.begin(), documents_.end(),
            [&](const auto& d) { return d->name() == name; }),
        documents_.end());

    try {
        App::GetApplication().closeDocument(name.c_str());
    } catch (...) {}
}

std::vector<std::string> CadSession::documentNames() const
{
    std::vector<std::string> names;
    names.reserve(documents_.size());
    for (const auto& d : documents_) {
        names.push_back(d->name());
    }
    return names;
}

} // namespace CADNC
