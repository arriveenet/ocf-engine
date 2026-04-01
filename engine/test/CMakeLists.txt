# ==================================================================================================
# Tests
# ==================================================================================================
add_executable(test_${TARGET}
    test_main.cpp
    test_result.cpp
)
target_link_libraries(test_${TARGET} PRIVATE gtest ocfengine)

set_target_properties(test_${TARGET} PROPERTIES FOLDER Tests)
