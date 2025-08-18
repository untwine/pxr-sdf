add_library(TestSdfNoAssetFileFormat SHARED plugin.cpp)
target_link_libraries(TestSdfNoAssetFileFormat PUBLIC sdf pxr::tf)
file(
    GENERATE
    OUTPUT "$<TARGET_FILE_DIR:TestSdfNoAssetFileFormat>/plugInfo_$<CONFIG>.json"
    INPUT plugInfo.json
)
