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

add_executable(testSdfUsdaFileFormatParsing testSdfUsdaFileFormatParsing.cpp)
target_link_libraries(testSdfUsdaFileFormatParsing PUBLIC sdf pxr::tf)
add_test(NAME testSdfUsdaFileFormatParsing COMMAND testSdfUsdaFileFormatParsing)
set_test_environment(testSdfUsdaFileFormatParsing)

add_executable(testSdfTextFile testSdfTextFile.cpp)
target_link_libraries(testSdfTextFile PUBLIC sdf pxr::tf)
add_test(NAME testSdfTextFile COMMAND testSdfTextFile)
set_test_environment(testSdfTextFile)

add_executable(testSdfTextFile_1.1 testSdfTextFile.cpp)
target_link_libraries(testSdfTextFile_1.1 PUBLIC sdf pxr::tf)
add_test(NAME testSdfTextFile_1.1 COMMAND testSdfTextFile_1.1)
set_test_environment(testSdfTextFile_1.1
    "USD_WRITE_NEW_USDA_FILES_AS_VERSION=1.1"
)

add_executable(testSdfFileVersion_Cpp testSdfFileVersion.cpp)
target_link_libraries(testSdfFileVersion_Cpp PUBLIC sdf pxr::tf)
add_test(NAME testSdfFileVersion_Cpp COMMAND testSdfFileVersion_Cpp)
set_test_environment(testSdfFileVersion_Cpp)

add_executable(testSdfIntegerCoding testSdfIntegerCoding.cpp)
target_link_libraries(testSdfIntegerCoding PUBLIC sdf)
add_test(NAME testSdfIntegerCoding COMMAND testSdfIntegerCoding)
set_test_environment(testSdfIntegerCoding)

add_executable(testSdfZipFile_CPP testSdfZipFile.cpp)
target_link_libraries(testSdfZipFile_CPP PUBLIC sdf pxr::tf pxr::ar pxr::arch)
add_test(NAME testSdfZipFile_CPP COMMAND testSdfZipFile_CPP)
set_test_environment(testSdfZipFile_CPP
    "DATA_PATH=$<SHELL_PATH:${_data}/testZipFile>"
)

add_executable(testSdfUsdzResolver testSdfUsdzResolver.cpp)
target_link_libraries(testSdfUsdzResolver PUBLIC sdf pxr::tf pxr::ar pxr::arch)
add_test(NAME testSdfUsdzResolver COMMAND testSdfUsdzResolver)
set_test_environment(testSdfUsdzResolver
    "DATA_PATH=$<SHELL_PATH:${_data}/testUsdzResolver>"
)

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
        TEST_PATHS
            testSdfAssetPath.py
            testSdfAttribute.py
            testSdfAttributeBlocking.py
            testSdfBatchNamespaceEdit.py
            testSdfColorConfig.py
            testSdfCopyUtils.py
            testSdfCustomLayerData.py
            testSdfDetachedLayer.py
            testSdfFileFormat.py
            testSdfFileFormatCapabilities.py
            testSdfLayer.py
            testSdfLayerMuting.py
            testSdfListOp.py
            testSdfParsing.py
            testSdfPath.py
            testSdfPath2.py
            testSdfPath2Construct.py
            testSdfPathExpression.py
            testSdfPathExpressionArray.py
            testSdfPayload.py
            testSdfPrim.py
            testSdfReference.py
            testSdfRelocates.py
            testSdfSpecHash.py
            testSdfSplineVersioning.py
            testSdfTargetFileFormat.py
            testSdfTimeCode.py
            testSdfTimeSamples.py
            testSdfTypes.py
            testSdfVariableExpression.py
            testSdfVariants.py
            testSdfZipFile.py
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
            "${_env}"
            "TEST_BATCH_NAMESPACE_EDIT_PATH=$<SHELL_PATH:${_data}/testBatchNamespaceEdit>"
            "TEST_COLOR_CONFIG_PATH=$<SHELL_PATH:${_data}/testColorConfig>"
            "TEST_CUSTOM_LAYER_DATA_PATH=$<SHELL_PATH:${_data}/testCustomLayerData>"
            "TEST_LAYER_MUTING_PATH=$<SHELL_PATH:${_data}/testLayerMuting>"
            "TEST_PARSING_PATH=$<SHELL_PATH:${_data}/testParsing>"
            "TEST_TARGET_FILE_FORMAT_PATH=$<SHELL_PATH:${_data}/testTargetFileFormat>"
            # Relative path handling is required for this test
            "TEST_ZIP_FILE_PATH=data/testZipFile"
        EXTRA_ARGS "-v"
        DEPENDS sdf pySdf testPlugins
    )
    
    pytest_discover_tests(
        TestSdfLegacyFileFormatAllow
        TEST_PATHS
            testSdfLegacyFileFormat.py
        LIBRARY_PATH_PREPEND
            $<TARGET_FILE_DIR:sdf>
            $<TARGET_FILE_DIR:pySdf>
            $<TARGET_FILE_DIR:pxr::tf>
            $<TARGET_FILE_DIR:pxr::pyTf>
        PYTHON_PATH_PREPEND
            "$<TARGET_FILE_DIR:pySdf>/../.."
            "$<TARGET_FILE_DIR:pxr::pyTf>/../.."
        TRIM_FROM_NAME "^test_"
        TRIM_FROM_FULL_NAME "^TestSdfLegacyFileFormat\."
        ENVIRONMENT
            "${_env}"
            "SDF_FILE_FORMAT_LEGACY_IMPORT=allow"
        EXTRA_ARGS "-v"
        DEPENDS sdf pySdf testPlugins
    )

    pytest_discover_tests(
        TestSdfLegacyFileFormatError
        TEST_PATHS
            testSdfLegacyFileFormat.py
        LIBRARY_PATH_PREPEND
            $<TARGET_FILE_DIR:sdf>
            $<TARGET_FILE_DIR:pySdf>
            $<TARGET_FILE_DIR:pxr::tf>
            $<TARGET_FILE_DIR:pxr::pyTf>
        PYTHON_PATH_PREPEND
            "$<TARGET_FILE_DIR:pySdf>/../.."
            "$<TARGET_FILE_DIR:pxr::pyTf>/../.."
        TRIM_FROM_NAME "^test_"
        TRIM_FROM_FULL_NAME "^TestSdfLegacyFileFormat\."
        ENVIRONMENT
            "${_env}"
            "SDF_FILE_FORMAT_LEGACY_IMPORT=error"
        EXTRA_ARGS "-v"
        DEPENDS sdf pySdf testPlugins
    )

    pytest_discover_tests(
        TestSdfZipFileLimits
        TEST_PATHS
            testSdfZipFileLimits.py
        LIBRARY_PATH_PREPEND
            $<TARGET_FILE_DIR:sdf>
            $<TARGET_FILE_DIR:pySdf>
            $<TARGET_FILE_DIR:pxr::tf>
            $<TARGET_FILE_DIR:pxr::pyTf>
        PYTHON_PATH_PREPEND
            "$<TARGET_FILE_DIR:pySdf>/../.."
            "$<TARGET_FILE_DIR:pxr::pyTf>/../.."
        TRIM_FROM_NAME "^test_"
        TRIM_FROM_FULL_NAME "^TestSdfZipFileLimits\."
        ENVIRONMENT
            "${_env}"
            "SDF_MAX_ZIPFILE_SIZE=195"
        EXTRA_ARGS "-v"
        DEPENDS sdf pySdf testPlugins
    )

    pytest_discover_tests(
        TestSdfEnvVar1
        TEST_PATHS
            testSdfDetachedLayer.py
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
        DEPENDS sdf pySdf testPlugins
    )

    pytest_discover_tests(
        TestSdfEnvVar2
        TEST_PATHS
            testSdfDetachedLayer.py
        LIBRARY_PATH_PREPEND
            $<TARGET_FILE_DIR:sdf>
            $<TARGET_FILE_DIR:pySdf>
            $<TARGET_FILE_DIR:pxr::tf>
            $<TARGET_FILE_DIR:pxr::pyTf>
        PYTHON_PATH_PREPEND
            "$<TARGET_FILE_DIR:pySdf>/../.."
            "$<TARGET_FILE_DIR:pxr::pyTf>/../.."
        TRIM_FROM_NAME "^test_"
        TRIM_FROM_NAME "^test_"
        TRIM_FROM_FULL_NAME "^TestSdf"
        ENVIRONMENT
            "${_env}"
            "SDF_LAYER_INCLUDE_IN_MEMORY=foo,bar"
            "SDF_LAYER_EXCLUDE_IN_MEMORY=baz,garply"
        EXTRA_ARGS "-v"
        DEPENDS sdf pySdf testPlugins
    )
endif()