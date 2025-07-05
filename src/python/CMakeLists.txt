add_library(pySdf SHARED
    module.cpp
    wrapArrayAssetPath.cpp
    wrapArrayPath.cpp
    wrapArrayPathExpression.cpp
    wrapArrayTimeCode.cpp
    wrapAssetPath.cpp
    wrapAttributeSpec.cpp
    wrapChangeBlock.cpp
    wrapCleanupEnabler.cpp
    wrapCopyUtils.cpp
    wrapFileFormat.cpp
    wrapLayer.cpp
    wrapLayerOffset.cpp
    wrapLayerTree.cpp
    wrapNamespaceEdit.cpp
    wrapNotice.cpp
    wrapOpaqueValue.cpp
    wrapPath.cpp
    wrapPathExpression.cpp
    wrapPathPattern.cpp
    wrapPayload.cpp
    wrapPredicateExpression.cpp
    wrapPredicateFunctionResult.cpp
    wrapPrimSpec.cpp
    wrapPropertySpec.cpp
    wrapPseudoRootSpec.cpp
    wrapRelationshipSpec.cpp
    wrapReference.cpp
    wrapSpec.cpp
    wrapVariableExpression.cpp
    wrapTimeCode.cpp
    wrapTypes.cpp
    wrapValueTypeName.cpp
    wrapVariantSpec.cpp
    wrapVariantSetSpec.cpp
)

set_target_properties(pySdf
    PROPERTIES
        PREFIX ""
        OUTPUT_NAME _sdf
        RUNTIME_OUTPUT_DIRECTORY "$<CONFIG>/pxr/Sdf"
        LIBRARY_OUTPUT_DIRECTORY "$<CONFIG>/pxr/Sdf"
        ARCHIVE_OUTPUT_DIRECTORY "$<CONFIG>/pxr/Sdf"
        POSITION_INDEPENDENT_CODE ON
)

target_include_directories(pySdf
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

target_link_libraries(pySdf
    PUBLIC
        sdf
        pxr::boost-python
)

if(SKBUILD AND NOT WIN32)
    set(_rpath_prefix "$ORIGIN")
    if(APPLE)
        set(_rpath_prefix "@loader_path")
    endif()
    set_target_properties(pySdf
        PROPERTIES
            INSTALL_RPATH "${_rpath_prefix}/../.libs"
            INSTALL_RPATH_USE_LINK_PATH TRUE
    )
endif()

if(WIN32)
    set_target_properties(pySdf PROPERTIES SUFFIX ".pyd")
elseif(APPLE)
    set_target_properties(pySdf PROPERTIES SUFFIX ".so")
endif()

if(MSVC)
    target_compile_options(pySdf PRIVATE /bigobj)
endif()

target_compile_definitions(
    pySdf
    PRIVATE
        BOOST_BIND_GLOBAL_PLACEHOLDERS
        BOOST_PYTHON_NO_PY_SIGNATURES
        MFB_PACKAGE_MODULE=Sdf
        MFB_PACKAGE_NAME=sdf
        MFB_ALT_PACKAGE_NAME=sdf
)

file(
    GENERATE
    OUTPUT "$<TARGET_FILE_DIR:pySdf>/__init__.py"
    INPUT "__init__.py"
)

install(
    TARGETS pySdf EXPORT ${PROJECT_NAME}
    LIBRARY DESTINATION ${CMAKE_INSTALL_PYTHON_LIBDIR}/pxr/Sdf
    RUNTIME DESTINATION ${CMAKE_INSTALL_PYTHON_LIBDIR}/pxr/Sdf
)
install(
    FILES __init__.py
    DESTINATION ${CMAKE_INSTALL_PYTHON_LIBDIR}/pxr/Sdf
)
