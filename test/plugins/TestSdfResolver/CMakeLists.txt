add_library(TestSdfResolver SHARED plugin.cpp)
target_link_libraries(TestSdfResolver PUBLIC sdf pxr::tf)
file(
    GENERATE
    OUTPUT "$<TARGET_FILE_DIR:TestSdfResolver>/plugInfo_$<CONFIG>.json"
    INPUT plugInfo.json
)
