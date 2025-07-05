add_library(TestSdfFileFormatCapabilities SHARED plugin.cpp)
target_link_libraries(TestSdfFileFormatCapabilities PUBLIC sdf pxr::tf)
file(
    GENERATE
    OUTPUT "$<TARGET_FILE_DIR:TestSdfFileFormatCapabilities>/plugInfo_$<CONFIG>.json"
    INPUT plugInfo.json
)
