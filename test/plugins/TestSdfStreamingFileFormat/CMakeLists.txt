add_library(TestSdfStreamingFileFormat SHARED plugin.cpp)
target_link_libraries(TestSdfStreamingFileFormat PUBLIC sdf pxr::tf)
file(
    GENERATE
    OUTPUT "$<TARGET_FILE_DIR:TestSdfStreamingFileFormat>/plugInfo_$<CONFIG>.json"
    INPUT plugInfo.json
)
