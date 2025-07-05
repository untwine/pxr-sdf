add_subdirectory(plugins)

set(_sdfPlugin "$<TARGET_FILE_DIR:sdf>/usd/sdf/resources/plugInfo_$<CONFIG>.json")
set(_env "PXR_PLUGINPATH_NAME=$<SHELL_PATH:${_sdfPlugin}>")

# Helper function to set test environment variables
function(set_test_environment NAME)

    # Update environment to extend the DLL search path on Windows.
    if (WIN32)
        set(DLL_DIRS $<TARGET_RUNTIME_DLL_DIRS:${NAME}>)
        set(_env "${_env};PATH=$<JOIN:$<SHELL_PATH:${DLL_DIRS}>,\\;>")
    endif()

    string (JOIN ";" args "${ARGN}")
    set_tests_properties(${NAME} PROPERTIES ENVIRONMENT "${args};${_env}")
endfunction()

set(_data "${CMAKE_CURRENT_SOURCE_DIR}/data")

add_executable(testSdfAbstractData_Cpp testSdfAbstractData.cpp)
target_link_libraries(testSdfAbstractData_Cpp PUBLIC sdf pxr::vt pxr::tf)
add_test(NAME testSdfAbstractData_Cpp COMMAND testSdfAbstractData_Cpp)
set_test_environment(testSdfAbstractData_Cpp)

add_executable(testSdfAttributeBlocking_Cpp testSdfAttributeBlocking.cpp)
target_link_libraries(testSdfAttributeBlocking_Cpp PUBLIC sdf)
add_test(NAME testSdfAttributeBlocking_Cpp COMMAND testSdfAttributeBlocking_Cpp)
set_test_environment(testSdfAttributeBlocking_Cpp)

add_executable(testSdfHardToReach testSdfHardToReach.cpp)
target_link_libraries(testSdfHardToReach PUBLIC sdf)
add_test(NAME testSdfHardToReach COMMAND testSdfHardToReach)
set_test_environment(testSdfHardToReach)

add_executable(testSdfLayerHints testSdfLayerHints.cpp)
target_link_libraries(testSdfLayerHints PUBLIC sdf)
add_test(NAME testSdfLayerHints COMMAND testSdfLayerHints)
set_test_environment(testSdfLayerHints
    "DATA_PATH=$<SHELL_PATH:${_data}/testLayerHints>"
)

add_executable(testSdfMetaDataPlugInfo testSdfMetaDataPlugInfo.cpp)
target_link_libraries(testSdfMetaDataPlugInfo PUBLIC sdf pxr::plug)
add_test(NAME testSdfMetaDataPlugInfo COMMAND testSdfMetaDataPlugInfo)
set_test_environment(testSdfMetaDataPlugInfo
    "PLUGIN_PATH=$<SHELL_PATH:${_data}/testMetaDataPlugInfo>"
)

add_executable(testSdfPathExpression_Cpp testSdfPathExpression.cpp)
target_link_libraries(testSdfPathExpression_Cpp PUBLIC sdf)
add_test(NAME testSdfPathExpression_Cpp COMMAND testSdfPathExpression_Cpp)
set_test_environment(testSdfPathExpression_Cpp)

add_executable(testSdfPathParser testSdfPathParser.cpp)
target_link_libraries(testSdfPathParser PUBLIC sdf)
add_test(NAME testSdfPathParser COMMAND testSdfPathParser)
set_test_environment(testSdfPathParser)

add_executable(testSdfPathTable testSdfPathTable.cpp)
target_link_libraries(testSdfPathTable PUBLIC sdf)
add_test(NAME testSdfPathTable COMMAND testSdfPathTable)
set_test_environment(testSdfPathTable)

add_executable(testSdfPathThreading testSdfPathThreading.cpp)
target_link_libraries(testSdfPathThreading PUBLIC sdf pxr::tf)
add_test(NAME testSdfPathThreading COMMAND testSdfPathThreading)
set_test_environment(testSdfPathThreading)

add_executable(testSdfPredicateExpression_Cpp testSdfPredicateExpression.cpp)
target_link_libraries(testSdfPredicateExpression_Cpp PUBLIC sdf pxr::tf)
add_test(NAME testSdfPredicateExpression_Cpp COMMAND testSdfPredicateExpression_Cpp)
set_test_environment(testSdfPredicateExpression_Cpp)

add_executable(testSdfTextFileFormatParsing testSdfTextFileFormatParsing.cpp)
target_link_libraries(testSdfTextFileFormatParsing PUBLIC sdf pxr::tf)
add_test(NAME testSdfTextFileFormatParsing COMMAND testSdfTextFileFormatParsing)
set_test_environment(testSdfTextFileFormatParsing)

if(BUILD_PYTHON_BINDINGS)
    set(_plugins
        ${_sdfPlugin}
        "$<TARGET_FILE_DIR:TestSdfFileFormatCapabilities>/plugInfo_$<CONFIG>.json"
        "$<TARGET_FILE_DIR:TestSdfLayerExceptionHandling>/plugInfo_$<CONFIG>.json"
        "$<TARGET_FILE_DIR:TestSdfNoAssetFileFormat>/plugInfo_$<CONFIG>.json"
        "$<TARGET_FILE_DIR:TestSdfResolver>/plugInfo_$<CONFIG>.json"
        "$<TARGET_FILE_DIR:TestSdfStreamingFileFormat>/plugInfo_$<CONFIG>.json"
        "$<TARGET_FILE_DIR:TestSdfTargetFileFormat>/plugInfo_$<CONFIG>.json"
    )

    if (WIN32)
        set(_env "PXR_PLUGINPATH_NAME=$<JOIN:$<SHELL_PATH:${_plugins}>,\\;>")
    else()
        set(_env "PXR_PLUGINPATH_NAME=$<JOIN:$<SHELL_PATH:${_plugins}>,:>")
    endif()

    pytest_discover_tests(
        TestSdf
        LIBRARY_PATH_PREPEND
            $<TARGET_FILE_DIR:sdf>
            $<TARGET_FILE_DIR:pySdf>
            $<TARGET_FILE_DIR:pxr::tf>
            $<TARGET_FILE_DIR:pxr::pyTf>
            $<TARGET_FILE_DIR:pxr::ar>
            $<TARGET_FILE_DIR:pxr::pyAr>
        PYTHON_PATH_PREPEND
            "$<TARGET_FILE_DIR:pySdf>/../.."
            "$<TARGET_FILE_DIR:pxr::pyTf>/../.."
            "$<TARGET_FILE_DIR:pxr::pyAr>/../.."
        TRIM_FROM_NAME "^test_"
        TRIM_FROM_FULL_NAME "^TestSdf"
        ENVIRONMENT
            "TEST_BATCH_NAMESPACE_EDIT_PATH=$<SHELL_PATH:${_data}/testBatchNamespaceEdit>"
            "TEST_COLOR_CONFIG_PATH=$<SHELL_PATH:${_data}/testColorConfig>"
            "TEST_CUSTOM_LAYER_DATA_PATH=$<SHELL_PATH:${_data}/testCustomLayerData>"
            "TEST_LAYER_MUTING_PATH=$<SHELL_PATH:${_data}/testLayerMuting>"
            "TEST_PARSING_PATH=$<SHELL_PATH:${_data}/testParsing>"
            "${_env}"
        EXTRA_ARGS "-v"
        DEPENDS sdf pySdf testPlugins
    )

    pytest_discover_tests(
        TestSdfEnvVar1
        LIBRARY_PATH_PREPEND
            $<TARGET_FILE_DIR:sdf>
            $<TARGET_FILE_DIR:pySdf>
            $<TARGET_FILE_DIR:pxr::tf>
            $<TARGET_FILE_DIR:pxr::pyTf>
        PYTHON_PATH_PREPEND
            "$<TARGET_FILE_DIR:pySdf>/../.."
            "$<TARGET_FILE_DIR:pxr::pyTf>/../.."
        TRIM_FROM_NAME "^test_"
        TRIM_FROM_FULL_NAME "^TestSdf"
        ENVIRONMENT
            "${_env}"
            "SDF_LAYER_INCLUDE_IN_MEMORY=*"
            "SDF_LAYER_EXCLUDE_IN_MEMORY=foo,bar"
        EXTRA_ARGS "-v"
        DISCOVERY_EXTRA_ARGS "-k=SdfDetachedLayer"
        DEPENDS sdf pySdf testPlugins
    )

    pytest_discover_tests(
        TestSdfEnvVar2
        LIBRARY_PATH_PREPEND
            $<TARGET_FILE_DIR:sdf>
            $<TARGET_FILE_DIR:pySdf>
            $<TARGET_FILE_DIR:pxr::tf>
            $<TARGET_FILE_DIR:pxr::pyTf>
        PYTHON_PATH_PREPEND
            "$<TARGET_FILE_DIR:pySdf>/../.."
            "$<TARGET_FILE_DIR:pxr::pyTf>/../.."
        TRIM_FROM_NAME "^test_"
        TRIM_FROM_FULL_NAME "^TestSdf"
        ENVIRONMENT
            "${_env}"
            "SDF_LAYER_INCLUDE_IN_MEMORY=foo,bar"
            "SDF_LAYER_EXCLUDE_IN_MEMORY=baz,garply"
        EXTRA_ARGS "-v"
        DISCOVERY_EXTRA_ARGS "-k=SdfDetachedLayer"
        DEPENDS sdf pySdf testPlugins
    )
endif()