add_executable(${PROJECT_NAME}
    main.cpp
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    SDL3::SDL3-static
    glm::glm
)
