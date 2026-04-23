# ==================================================================================================
# Tests
# ==================================================================================================
add_executable(test_${TARGET}
    test_geometric.cpp
    test_main.cpp
    test_mat2.cpp
    test_mat3.cpp
    test_mat4.cpp
    test_matrix_transform.cpp
    test_quat.cpp
    test_rect.cpp
    test_result.cpp
    test_vec.cpp
)
target_link_libraries(test_${TARGET} PRIVATE gtest ocfengine)

set_target_properties(test_${TARGET} PROPERTIES FOLDER Tests)
