# CADNC Architecture

## Overview

CADNC is a layered CAD-CAM application built on FreeCAD's core modules.

```
┌─────────────────────────────────────────┐
│           UI Shell (QML)                │
│   Main.qml, Toolbars, Panels           │
├─────────────────────────────────────────┤
│         Adapter Layer (C++)             │
│  CadSession  CadDocument               │
│  SketchFacade  PartFacade              │
│  SelectionFacade  PropertyFacade       │
├──────────┬──────────┬───────────────────┤
│ FreeCAD  │   CAM    │    Nesting        │
│ Modules  │  Module  │    Module         │
│ (LGPL)   │          │                   │
├──────────┴──────────┴───────────────────┤
│        OpenCASCADE (OCCT) Kernel        │
└─────────────────────────────────────────┘
```

## Layer Responsibilities

### UI Shell (ui/)
- QML-based modern desktop interface
- Workbench tabs: Part, Sketch, CAM, Nesting
- Model tree, property panel, toolbars
- No direct access to FreeCAD types

### Adapter Layer (adapter/)
- Thin C++ facades wrapping FreeCAD objects
- Translates UI commands to FreeCAD operations
- Translates FreeCAD state to UI-friendly data
- Isolates UI from backend changes

### FreeCAD Modules (freecad/)
- **Base**: Math, I/O, type system, Python
- **App**: Document, Property, Expression, Transaction
- **Part/App**: TopoShape, Geometry, Part features
- **Sketcher/App**: SketchObject, Constraints, planegcs
- **PartDesign/App**: Pad, Pocket, Fillet, Chamfer, etc.

### CAM Module (cam/)
- Toolpath generation algorithms
- G-Code synthesis and parsing
- Post-processors (CODESYS, generic ISO 6983)
- Tool library management

### Nesting Module (nesting/)
- Sheet nesting algorithms (BLF, bounding box)
- Part placement optimization

### OCCT Kernel
- Geometry kernel used by FreeCAD modules
- BRep topology, STEP/IGES I/O, meshing

## Key Design Rules

1. UI code MUST NOT import or reference FreeCAD headers
2. Adapter facades MUST NOT expose FreeCAD types in their public API
3. FreeCAD modules SHOULD be kept as close to upstream as possible
4. CAM and Nesting modules are independent of FreeCAD
5. All thread-sensitive operations go through the adapter layer
