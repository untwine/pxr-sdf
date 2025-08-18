add_library(TestSdfLayerExceptionHandling SHARED plugin.cpp)
target_link_libraries(TestSdfLayerExceptionHandling PUBLIC sdf pxr::tf)
file(
    GENERATE
    OUTPUT "$<TARGET_FILE_DIR:TestSdfLayerExceptionHandling>/plugInfo_$<CONFIG>.json"
    INPUT plugInfo.json
)
