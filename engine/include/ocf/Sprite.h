#pragma once
#include "ocf/Component.h"
#include <glm/glm.hpp>

namespace ocf {

class Program;
class VertexArrayObject;

class Sprite : public Component {
public:
    Sprite();
    ~Sprite() override;
    void update(float deltaTime) override;

protected:
    Program* m_program = nullptr;
    VertexArrayObject* m_vertexArrayObject = nullptr;
    glm::vec2 m_size;
};

} // namespace ocf
