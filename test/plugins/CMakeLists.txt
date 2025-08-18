add_subdirectory(TestSdfFileFormatCapabilities)
add_subdirectory(TestSdfLayerExceptionHandling)
add_subdirectory(TestSdfNoAssetFileFormat)
add_subdirectory(TestSdfResolver)
add_subdirectory(TestSdfStreamingFileFormat)
add_subdirectory(TestSdfTargetFileFormat)

add_library(testPlugins INTERFACE)
add_dependencies(testPlugins
    TestSdfFileFormatCapabilities
    TestSdfLayerExceptionHandling
    TestSdfNoAssetFileFormat
    TestSdfResolver
    TestSdfStreamingFileFormat
    TestSdfTargetFileFormat
)
