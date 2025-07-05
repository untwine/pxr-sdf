add_library(sdf
    pxr/sdf/abstractData.cpp
    pxr/sdf/allowed.cpp
    pxr/sdf/assetPath.cpp
    pxr/sdf/assetPathResolver.cpp
    pxr/sdf/attributeSpec.cpp
    pxr/sdf/changeBlock.cpp
    pxr/sdf/changeList.cpp
    pxr/sdf/changeManager.cpp
    pxr/sdf/children.cpp
    pxr/sdf/childrenPolicies.cpp
    pxr/sdf/childrenProxy.cpp
    pxr/sdf/childrenUtils.cpp
    pxr/sdf/childrenView.cpp
    pxr/sdf/cleanupEnabler.cpp
    pxr/sdf/cleanupTracker.cpp
    pxr/sdf/connectionListEditor.cpp
    pxr/sdf/copyUtils.cpp
    pxr/sdf/data.cpp
    pxr/sdf/debugCodes.cpp
    pxr/sdf/declareHandles.cpp
    pxr/sdf/fileFormat.cpp
    pxr/sdf/fileFormatRegistry.cpp
    pxr/sdf/fileIO.cpp
    pxr/sdf/fileIO_Common.cpp
    pxr/sdf/identity.cpp
    pxr/sdf/layer.cpp
    pxr/sdf/layerOffset.cpp
    pxr/sdf/layerRegistry.cpp
    pxr/sdf/layerStateDelegate.cpp
    pxr/sdf/layerTree.cpp
    pxr/sdf/layerUtils.cpp
    pxr/sdf/listEditor.cpp
    pxr/sdf/listEditorProxy.cpp
    pxr/sdf/listOp.cpp
    pxr/sdf/listOpListEditor.cpp
    pxr/sdf/listProxy.cpp
    pxr/sdf/mapEditProxy.cpp
    pxr/sdf/mapEditor.cpp
    pxr/sdf/namespaceEdit.cpp
    pxr/sdf/notice.cpp
    pxr/sdf/opaqueValue.cpp
    pxr/sdf/parserHelpers.cpp
    pxr/sdf/parserValueContext.cpp
    pxr/sdf/path.cpp
    pxr/sdf/pathExpression.cpp
    pxr/sdf/pathExpressionEval.cpp
    pxr/sdf/pathNode.cpp
    pxr/sdf/pathParser.cpp
    pxr/sdf/pathPattern.cpp
    pxr/sdf/pathTable.cpp
    pxr/sdf/payload.cpp
    pxr/sdf/pool.cpp
    pxr/sdf/predicateExpression.cpp
    pxr/sdf/predicateLibrary.cpp
    pxr/sdf/primSpec.cpp
    pxr/sdf/propertySpec.cpp
    pxr/sdf/proxyPolicies.cpp
    pxr/sdf/proxyTypes.cpp
    pxr/sdf/pseudoRootSpec.cpp
    pxr/sdf/reference.cpp
    pxr/sdf/relationshipSpec.cpp
    pxr/sdf/schema.cpp
    pxr/sdf/site.cpp
    pxr/sdf/siteUtils.cpp
    pxr/sdf/spec.cpp
    pxr/sdf/specType.cpp
    pxr/sdf/subLayerListEditor.cpp
    pxr/sdf/textFileFormat.cpp
    pxr/sdf/textFileFormatParser.cpp
    pxr/sdf/textParserContext.cpp
    pxr/sdf/textParserHelpers.cpp
    pxr/sdf/timeCode.cpp
    pxr/sdf/tokens.cpp
    pxr/sdf/types.cpp
    pxr/sdf/valueTypeName.cpp
    pxr/sdf/valueTypeRegistry.cpp
    pxr/sdf/variableExpression.cpp
    pxr/sdf/variableExpressionImpl.cpp
    pxr/sdf/variableExpressionParser.cpp
    pxr/sdf/variantSetSpec.cpp
    pxr/sdf/variantSpec.cpp
    pxr/sdf/vectorListEditor.cpp
)

target_include_directories(sdf
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

target_link_libraries(sdf
    PUBLIC
        pxr::arch
        pxr::tf
        pxr::gf
        pxr::pegtl
        pxr::trace
        pxr::ts
        pxr::vt
        pxr::work
        pxr::ar
        pxr::plug
        TBB::tbb
)

if(BUILD_PYTHON_BINDINGS)
    target_sources(sdf
        PRIVATE
            pxr/sdf/moduleDeps.cpp
            pxr/sdf/pyChildrenProxy.cpp
            pxr/sdf/pyChildrenView.cpp
            pxr/sdf/pyListEditorProxy.cpp
            pxr/sdf/pyListOp.cpp
            pxr/sdf/pyListProxy.cpp
            pxr/sdf/pyMapEditProxy.cpp
            pxr/sdf/pySpec.cpp
            pxr/sdf/pyUtils.cpp
    )

    target_link_libraries(sdf
        PUBLIC
            pxr::boost-python
    )

    add_subdirectory(python)
endif()

if (ENABLE_PRECOMPILED_HEADERS)
    target_precompile_headers(sdf
        PRIVATE
            pxr/sdf/pch.h
    )
endif()

if(MSVC)
    target_compile_options(sdf PRIVATE /bigobj)
endif()

set_target_properties(sdf
    PROPERTIES
        POSITION_INDEPENDENT_CODE ON
        OUTPUT_NAME "PxrSdf"
)

target_compile_definitions(sdf
    PRIVATE
        SDF_EXPORTS=1
        MFB_PACKAGE_MODULE=Sdf
        MFB_PACKAGE_NAME=sdf
        MFB_ALT_PACKAGE_NAME=sdf
)

if(SKBUILD)
    set(_lib_prefix "../../.libs/")
    set(_build_dir "../pluginfo/sdf/resources")
else()
    set(_lib_prefix "../../")
    set(_build_dir "usd/sdf/resources")
endif()

set(PLUG_INFO_LIBRARY_PATH "${_lib_prefix}$<TARGET_FILE_NAME:sdf>")
configure_file(pxr/sdf/plugInfo.json plugInfo.json @ONLY)

file(
    GENERATE
    OUTPUT "$<TARGET_FILE_DIR:sdf>/usd/sdf/resources/plugInfo_$<CONFIG>.json"
    INPUT "${CMAKE_CURRENT_BINARY_DIR}/plugInfo.json"
)

install(
    FILES "$<TARGET_FILE_DIR:sdf>/usd/sdf/resources/plugInfo_$<CONFIG>.json"
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/${_build_dir}"
    RENAME plugInfo.json
)

install(
    TARGETS sdf EXPORT ${PROJECT_NAME}
    RUNTIME DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

install(
    FILES
        pxr/sdf/abstractData.h
        pxr/sdf/accessorHelpers.h
        pxr/sdf/allowed.h
        pxr/sdf/api.h
        pxr/sdf/assetPath.h
        pxr/sdf/attributeSpec.h
        pxr/sdf/changeBlock.h
        pxr/sdf/changeList.h
        pxr/sdf/children.h
        pxr/sdf/childrenPolicies.h
        pxr/sdf/childrenProxy.h
        pxr/sdf/childrenUtils.h
        pxr/sdf/childrenView.h
        pxr/sdf/cleanupEnabler.h
        pxr/sdf/copyUtils.h
        pxr/sdf/data.h
        pxr/sdf/declareHandles.h
        pxr/sdf/declareSpec.h
        pxr/sdf/fileFormat.h
        pxr/sdf/identity.h
        pxr/sdf/layer.h
        pxr/sdf/layerHints.h
        pxr/sdf/layerOffset.h
        pxr/sdf/layerStateDelegate.h
        pxr/sdf/layerTree.h
        pxr/sdf/layerUtils.h
        pxr/sdf/listEditor.h
        pxr/sdf/listEditorProxy.h
        pxr/sdf/listOp.h
        pxr/sdf/listProxy.h
        pxr/sdf/mapEditProxy.h
        pxr/sdf/mapEditor.h
        pxr/sdf/namespaceEdit.h
        pxr/sdf/notice.h
        pxr/sdf/opaqueValue.h
        pxr/sdf/path.h
        pxr/sdf/pathExpression.h
        pxr/sdf/pathExpressionEval.h
        pxr/sdf/pathNode.h
        pxr/sdf/pathPattern.h
        pxr/sdf/pathPatternParser.h
        pxr/sdf/pathTable.h
        pxr/sdf/payload.h
        pxr/sdf/pool.h
        pxr/sdf/predicateExpression.h
        pxr/sdf/predicateExpressionParser.h
        pxr/sdf/predicateLibrary.h
        pxr/sdf/predicateProgram.h
        pxr/sdf/primSpec.h
        pxr/sdf/propertySpec.h
        pxr/sdf/proxyPolicies.h
        pxr/sdf/proxyTypes.h
        pxr/sdf/pseudoRootSpec.h
        pxr/sdf/reference.h
        pxr/sdf/relationshipSpec.h
        pxr/sdf/schema.h
        pxr/sdf/schemaTypeRegistration.h
        pxr/sdf/site.h
        pxr/sdf/siteUtils.h
        pxr/sdf/spec.h
        pxr/sdf/specType.h
        pxr/sdf/textFileFormat.h
        pxr/sdf/timeCode.h
        pxr/sdf/tokens.h
        pxr/sdf/types.h
        pxr/sdf/valueTypeName.h
        pxr/sdf/variableExpression.h
        pxr/sdf/variantSetSpec.h
        pxr/sdf/variantSpec.h
    DESTINATION
        ${CMAKE_INSTALL_INCLUDEDIR}/pxr/sdf
)

if(BUILD_PYTHON_BINDINGS)
    install(
        FILES
            pxr/sdf/pyChildrenProxy.h
            pxr/sdf/pyChildrenView.h
            pxr/sdf/pyListEditorProxy.h
            pxr/sdf/pyListOp.h
            pxr/sdf/pyListProxy.h
            pxr/sdf/pyMapEditProxy.h
            pxr/sdf/pySpec.h
            pxr/sdf/pyUtils.h
        DESTINATION
            ${CMAKE_INSTALL_INCLUDEDIR}/pxr/sdf
    )
endif()

install(EXPORT ${PROJECT_NAME}
    FILE pxr-sdf-targets.cmake
    NAMESPACE pxr::
    DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/cmake/pxr-sdf
)
