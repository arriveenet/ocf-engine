#include <ocf/Camera.h>
#include <ocf/Engine.h>
#include <ocf/FileUtils.h>
#include <ocf/IndexBuffer.h>
#include <ocf/Node.h>
#include <ocf/Program.h>
#include <ocf/RenderCommand.h>
#include <ocf/Renderer.h>
#include <ocf/Scene.h>
#include <ocf/Types.h>
#include <ocf/VertexArrayObject.h>
#include <ocf/VertexBuffer.h>
#include <stdlib.h>
#define _CRTDBG_MAP_ALLOC
#include <cstdlib>
#ifdef _WIN32
#include <crtdbg.h>
#endif

using namespace ocf;

// A simple node that owns and renders a colored quad
class QuadNode : public Node {
public:
    QuadNode()
    {
        static Vertex2fC4f vertices[4] = {
            {{-0.5f, -0.5f}, {1.0f, 0.0f, 0.0f, 1.0f}},
            {{ 0.5f, -0.5f}, {0.0f, 1.0f, 0.0f, 1.0f}},
            {{-0.5f,  0.5f}, {0.0f, 0.0f, 1.0f, 1.0f}},
            {{ 0.5f,  0.5f}, {1.0f, 1.0f, 1.0f, 1.0f}}
        };
        static uint16_t indices[6] = {0, 1, 2, 2, 1, 3};

        const auto vertPath = FileUtils::getInstance()->fullPathForFilename("shaders/basic.vert");
        const auto fragPath = FileUtils::getInstance()->fullPathForFilename("shaders/basic.frag");
        m_program = Program::Builder()
            .vertexShader(vertPath)
            .fragmentShader(fragPath)
            .build();

        VertexBuffer* vb = VertexBuffer::Builder()
            .bufferCount(1).vertexCount(4)
            .attribute(VertexAttribute::POSITION, ElementType::FLOAT2, sizeof(Vertex2fC4f), offsetof(Vertex2fC4f, position))
            .attribute(VertexAttribute::COLOR,    ElementType::FLOAT4, sizeof(Vertex2fC4f), offsetof(Vertex2fC4f, color))
            .build();
        vb->setBufferData(vertices, sizeof(vertices), 0);

        IndexBuffer* ib = IndexBuffer::Builder()
            .indexCount(6)
            .build();
        ib->setBufferData(indices, sizeof(indices), 0);

        m_vertexArrayObject = VertexArrayObject::Builder()
            .geometry(PrimitiveType::TRIANGLES, vb, ib, 0, 6)
            .build();
    }

    ~QuadNode() override
    {
        delete m_program;
        delete m_vertexArrayObject;
    }

    void render(Renderer* renderer) override
    {
        RenderCommand command;
        command.program = m_program;
        command.vertexArrayObject = m_vertexArrayObject;
        renderer->pushCommand(command);

        Node::render(renderer);
    }

private:
    Program* m_program = nullptr;
    VertexArrayObject* m_vertexArrayObject = nullptr;
};

int main(int argc, char* argv[])
{
#ifdef _WIN32
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

    Scene scene;

    Engine engine;

    if (!engine.init("Title", 800, 600, &scene)) {
        return 1;
    }

    // Create a camera and register it with the scene.
    // The camera is valid for the entire lifetime of the scene (both are local to main).
    Camera camera;
    camera.setCameraFlag(CameraFlag::Default);
    scene.addCamera(&camera);

    // Add a quad node to the scene.
    // Ownership is transferred to the scene's root node, which deletes it in its destructor.
    QuadNode* quadNode = new QuadNode();
    scene.getRoot()->addChild(quadNode);

    engine.run();
    engine.terminate();

    return 0;
}
