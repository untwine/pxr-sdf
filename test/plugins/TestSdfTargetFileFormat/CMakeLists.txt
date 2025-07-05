add_library(TestSdfTargetFileFormat SHARED plugin.cpp)
target_link_libraries(TestSdfTargetFileFormat PUBLIC sdf pxr::tf)
file(
    GENERATE
    OUTPUT "$<TARGET_FILE_DIR:TestSdfTargetFileFormat>/plugInfo_$<CONFIG>.json"
    INPUT plugInfo.json
)
