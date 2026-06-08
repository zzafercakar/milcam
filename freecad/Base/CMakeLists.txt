# SPDX-License-Identifier: LGPL-2.1-or-later
# FreeCADBase — core library: math, I/O, type system, units, Python integration

add_library(FreeCADBase SHARED)

# ---------------------------------------------------------------------------
# Compile definitions
# ---------------------------------------------------------------------------
# FreeCADBase_EXPORTS triggers BaseExport macro in FCGlobal.h (dllexport on Windows)
target_compile_definitions(FreeCADBase PRIVATE FreeCADBase_EXPORTS QT_NO_KEYWORDS)
# Keep Py::Exception 2-arg constructors (PyObject*, string) from Exception.hxx
target_compile_definitions(FreeCADBase PUBLIC PYCXX_6_2_COMPATIBILITY)
# zipios++ needs these to use modern std::ostringstream instead of deprecated ostrstream
target_compile_definitions(FreeCADBase PRIVATE HAVE_STD_IOSTREAM=1 USE_STD_IOSTREAM=1)

# ---------------------------------------------------------------------------
# Include directories
# ---------------------------------------------------------------------------
target_include_directories(FreeCADBase
    PUBLIC
        # So downstream can do #include <Base/Vector3D.h>
        "${FREECAD_SRC_DIR}"
        # FCConfig.h and FCGlobal.h live in freecad/
        "${FREECAD_SRC_DIR}"
    PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}"
        "${CMAKE_CURRENT_BINARY_DIR}"
)

target_include_directories(FreeCADBase SYSTEM PUBLIC
    ${Boost_INCLUDE_DIRS}
    ${PYCXX_INCLUDE_DIRS}
    ${Python3_INCLUDE_DIRS}
    ${XercesC_INCLUDE_DIRS}
    ${ZLIB_INCLUDE_DIR}
    ${ZIPIOS_INCLUDES}
)

# ICU is optional
if(ICU_FOUND)
    target_include_directories(FreeCADBase SYSTEM PUBLIC ${ICU_INCLUDE_DIRS})
endif()

# ---------------------------------------------------------------------------
# Pre-generated Python binding files
# ---------------------------------------------------------------------------
# These were generated from .pyi files using FreeCAD's generate.py tool.
# They live in the source tree to avoid needing the tool at build time.
set(GENERATED_PY_SOURCES "")
generate_from_py(Type)
generate_from_py(BaseClass)
generate_from_py(BoundBox)
generate_from_py(CoordinateSystem)
generate_from_py(Persistence)
generate_from_py(Vector)
generate_from_py(Matrix)
generate_from_py(Rotation)
generate_from_py(Placement)
generate_from_py(Axis)
generate_from_py(Unit)
generate_from_py(Quantity)
generate_from_py(Precision)

# Parameter.inl — XSD schema embedded as C++ string
generate_from_any(Parameter.xsd Parameter.inl xmlSchemeString)

# ---------------------------------------------------------------------------
# Source files
# ---------------------------------------------------------------------------
set(FreeCADBase_CPP_SRCS
    Axis.cpp
    AxisPyImp.cpp
    Base64.cpp
    BaseClass.cpp
    BaseClassPyImp.cpp
    BindingManager.cpp
    BoundBoxPyImp.cpp
    Builder3D.cpp
    Console.cpp
    ConsoleObserver.cpp
    Color.cpp
    CoordinateSystem.cpp
    CoordinateSystemPyImp.cpp
    DualQuaternion.cpp
    Exception.cpp
    ExceptionFactory.cpp
    Factory.cpp
    FileInfo.cpp
    FileLock.cpp
    GeometryPyCXX.cpp
    Handle.cpp
    InputSource.cpp
    Interpreter.cpp
    Matrix.cpp
    MatrixPyImp.cpp
    Observer.cpp
    Parameter.cpp
    ParameterPy.cpp
    Persistence.cpp
    PersistencePyImp.cpp
    Placement.cpp
    PlacementPyImp.cpp
    PrecisionPyImp.cpp
    ProgressIndicator.cpp
    ProgressIndicatorPy.cpp
    PyExport.cpp
    PyObjectBase.cpp
    PythonTypeExt.cpp
    Reader.cpp
    Rotation.cpp
    RotationPyImp.cpp
    Sequencer.cpp
    ServiceProvider.cpp
    SmartPtrPy.cpp
    Stream.cpp
    Swap.cpp
    swigpyrun.cpp
    SystemHandler.cpp
    Tools.cpp
    Tools2D.cpp
    Tools3D.cpp
    Translation.cpp
    Translate.cpp
    Type.cpp
    TypePyImp.cpp
    UniqueNameManager.cpp
    Uuid.cpp
    Vector3D.cpp
    VectorPyImp.cpp
    ViewProj.cpp
    Writer.cpp
    XMLTools.cpp
    ZipHeader.cpp
)

set(FreeCADBase_UNITAPI_SRCS
    UnitsApi.cpp
    UnitsApiPy.cpp
    UnitsSchema.cpp
    UnitsSchemas.cpp
    Quantity.cpp
    QuantityPyImp.cpp
    Unit.cpp
    UnitPyImp.cpp
)

# Collect all sources
set(FreeCADBase_ALL_SRCS
    ${PYCXX_SOURCES}
    ${FreeCADBase_CPP_SRCS}
    ${FreeCADBase_UNITAPI_SRCS}
    ${GENERATED_PY_SOURCES}
    ${zipios_SRCS}
    PyTools.c
)

target_sources(FreeCADBase PRIVATE ${FreeCADBase_ALL_SRCS})

# ---------------------------------------------------------------------------
# Link libraries
# ---------------------------------------------------------------------------
set(FreeCADBase_LIBS
    Qt6::Core
    ${XercesC_LIBRARIES}
    ${Boost_LIBRARIES}
    ${ZLIB_LIBRARIES}
    Python3::Python
    libfastsignals
    fmt::fmt
)

if(ICU_FOUND)
    list(APPEND FreeCADBase_LIBS ${ICU_LIBRARIES})
endif()

# Linux-specific: util and dl needed for Python embedding
if(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
    list(APPEND FreeCADBase_LIBS -lutil -ldl)
endif()

target_link_libraries(FreeCADBase PUBLIC ${FreeCADBase_LIBS})

# ---------------------------------------------------------------------------
# Compiler settings
# ---------------------------------------------------------------------------
target_compile_features(FreeCADBase PUBLIC cxx_std_20)
set_target_properties(FreeCADBase PROPERTIES
    POSITION_INDEPENDENT_CODE ON
    # Output to build/lib/ so all .so files are together
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
)

# Suppress warnings in FreeCAD code (upstream — we don't modify)
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(FreeCADBase PRIVATE
        -Wno-unused-parameter
        -Wno-missing-field-initializers
        -Wno-deprecated-declarations
        -Wno-sign-compare
        -Wno-write-strings
    )
endif()

# ---------------------------------------------------------------------------
# Install
# ---------------------------------------------------------------------------
install(TARGETS FreeCADBase
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
