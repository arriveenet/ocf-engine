add_library(${PROJECT_NAME} STATIC
    ocf-engine.cpp
)

target_include_directories(${PROJECT_NAME} PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
)

target_link_libraries(${PROJECT_NAME} PUBLIC
    SDL3::SDL3-static
    glm::glm
)
