set(HDRS
    include/ocf/Component.h
    include/ocf/Engine.h
    include/ocf/FileUtils.h
    include/ocf/Node.h
    include/ocf/Program.h
    include/ocf/Renderer.h
    include/ocf/RendererEnums.h
    include/ocf/Scene.h
    include/ocf/Types.h
    include/ocf/Window.h
    include/ocf/VertexArrayObject.h
    include/ocf/VertexBuffer.h
    include/ocf/IndexBuffer.h
    include/ocf/OpenGLUtility.h
)

set(SRCS
    src/Component.cpp
    src/Engine.cpp
    src/FileUtils.cpp
    src/Node.cpp
    src/Program.cpp
    src/Renderer.cpp
    src/Scene.cpp
    src/Window.cpp
    src/VertexArrayObject.cpp
    src/VertexBuffer.cpp
    src/IndexBuffer.cpp
    src/OpenGLUtility.cpp
)

add_library(${PROJECT_NAME} STATIC
    ${HDRS} ${SRCS}
)

target_include_directories(${PROJECT_NAME} PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

target_link_libraries(${PROJECT_NAME} PUBLIC
    SDL3::SDL3-static
    glm::glm
    glad
)
