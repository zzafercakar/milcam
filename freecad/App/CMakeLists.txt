# SPDX-License-Identifier: LGPL-2.1-or-later
# FreeCADApp — document, property, expression, transaction system

add_library(FreeCADApp SHARED)

# ---------------------------------------------------------------------------
# Compile definitions
# ---------------------------------------------------------------------------
target_compile_definitions(FreeCADApp PRIVATE FreeCADApp_EXPORTS QT_NO_KEYWORDS)

# ---------------------------------------------------------------------------
# Include directories
# ---------------------------------------------------------------------------
target_include_directories(FreeCADApp
    PUBLIC
        "${FREECAD_SRC_DIR}"
    PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}"
        "${CMAKE_CURRENT_BINARY_DIR}"
)

# ---------------------------------------------------------------------------
# Pre-generated Python binding files (from .pyi via generate.py)
# ---------------------------------------------------------------------------
set(GENERATED_PY_SOURCES "")
generate_from_py(ApplicationDirectories)
generate_from_py(Document)
generate_from_py(DocumentObject)
generate_from_py(Extension)
generate_from_py(ExtensionContainer)
generate_from_py(DocumentObjectExtension)
generate_from_py(GroupExtension)
generate_from_py(LinkBaseExtension)
generate_from_py(DocumentObjectGroup)
generate_from_py(GeoFeature)
generate_from_py(GeoFeatureGroupExtension)
generate_from_py(SuppressibleExtension)
generate_from_py(Metadata)
generate_from_py(OriginGroupExtension)
generate_from_py(Part)
generate_from_py(StringHasher)
generate_from_py(StringID)
generate_from_py(ComplexGeoData)
generate_from_py(PropertyContainer)
generate_from_py(Material)
generate_from_py(MeasureManager)

# ---------------------------------------------------------------------------
# Source files — every .cpp except generated *Py.cpp (included via *PyImp.cpp)
# ---------------------------------------------------------------------------
set(FreeCADApp_SRCS
    Annotation.cpp
    Application.cpp
    ApplicationDirectories.cpp
    ApplicationDirectoriesPyImp.cpp
    ApplicationPy.cpp
    AutoTransaction.cpp
    BackupPolicy.cpp
    Branding.cpp
    CleanupProcess.cpp
    ColorModel.cpp
    ComplexGeoData.cpp
    ComplexGeoDataPyImp.cpp
    ConsoleQtBridge.cpp
    Datums.cpp
    Document.cpp
    DocumentObject.cpp
    DocumentObjectExtension.cpp
    DocumentObjectExtensionPyImp.cpp
    DocumentObjectFileIncluded.cpp
    DocumentObjectGroup.cpp
    DocumentObjectGroupPyImp.cpp
    DocumentObjectPyImp.cpp
    DocumentObserver.cpp
    DocumentObserverPython.cpp
    DocumentPyImp.cpp
    DynamicProperty.cpp
    ElementMap.cpp
    ElementNamingUtils.cpp
    Enumeration.cpp
    Expression.cpp
    ExpressionTokenizer.cpp
    ExtensionContainer.cpp
    ExtensionContainerPyImp.cpp
    Extension.cpp
    ExtensionPyImp.cpp
    FeaturePython.cpp
    FeatureTest.cpp
    GeoFeature.cpp
    GeoFeatureGroupExtension.cpp
    GeoFeatureGroupExtensionPyImp.cpp
    GeoFeaturePyImp.cpp
    Graphviz.cpp
    GroupExtension.cpp
    GroupExtensionPyImp.cpp
    ImagePlane.cpp
    IndexedName.cpp
    InventorObject.cpp
    LinkBaseExtensionPyImp.cpp
    Link.cpp
    MappedElement.cpp
    MappedName.cpp
    Material.cpp
    MaterialObject.cpp
    MaterialPyImp.cpp
    MeasureManager.cpp
    MeasureManagerPyImp.cpp
    MergeDocuments.cpp
    Metadata.cpp
    MetadataPyImp.cpp
    ObjectIdentifier.cpp
    Origin.cpp
    OriginGroupExtension.cpp
    OriginGroupExtensionPyImp.cpp
    Part.cpp
    PartPyImp.cpp
    Path.cpp
    Placement.cpp
    ProgramInformation.cpp
    ProjectFile.cpp
    PropertyContainer.cpp
    PropertyContainerPyImp.cpp
    Property.cpp
    PropertyExpressionEngine.cpp
    PropertyFile.cpp
    PropertyGeo.cpp
    PropertyLinks.cpp
    PropertyPythonObject.cpp
    PropertyStandard.cpp
    PropertyUnits.cpp
    Range.cpp
    SafeMode.cpp
    Services.cpp
    StringHasher.cpp
    StringHasherPyImp.cpp
    StringIDPyImp.cpp
    SuppressibleExtension.cpp
    SuppressibleExtensionPyImp.cpp
    TextDocument.cpp
    TransactionalObject.cpp
    Transactions.cpp
    TranslationQtBridge.cpp
    VarSet.cpp
    VRMLObject.cpp
)

target_sources(FreeCADApp PRIVATE
    ${FreeCADApp_SRCS}
    ${GENERATED_PY_SOURCES}
)

# ---------------------------------------------------------------------------
# Link libraries
# ---------------------------------------------------------------------------
target_link_libraries(FreeCADApp PUBLIC
    FreeCADBase
    Qt6::Core
    Qt6::Xml
    ${Boost_LIBRARIES}
    fmt::fmt
)

# ---------------------------------------------------------------------------
# Compiler settings
# ---------------------------------------------------------------------------
target_compile_features(FreeCADApp PUBLIC cxx_std_20)
set_target_properties(FreeCADApp PROPERTIES
    POSITION_INDEPENDENT_CODE ON
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
)

# Suppress warnings in FreeCAD upstream code
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(FreeCADApp PRIVATE
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
install(TARGETS FreeCADApp
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
