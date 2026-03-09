#pragma once
#include <glm/glm.hpp>
#include <stdint.h>

namespace ocf {

enum class PixelFormat : uint8_t {
    GRAY,
    RGB,
    RGBA,
    NONE
};

enum class ButtonState : uint8_t{
    None, 
    Pressed,
    Released,
    Hold
};

enum class CameraFlag : uint16_t {
    Default = 1,
    User1 = 1 << 1,
    User2 = 1 << 2,
    User3 = 1 << 3,
    User4 = 1 << 4,
};

struct Vertex2fT2f {
  glm::vec2 position;
  glm::vec2 texCoord;
};

struct Vertex2fC4f {
  glm::vec2 position;
  glm::vec4 color;
};

struct Vertex3fT2f {
  glm::vec3 position;
  glm::vec2 texCoord;
};

struct Vertex3fC4f {
  glm::vec3 position;
  glm::vec4 color;
};

struct Vertex3fC3fT2f {
  glm::vec3 position;
  glm::vec3 color;
  glm::vec2 texCoord;
};

struct QuadV3fT2f {
  Vertex3fT2f topLeft;
  Vertex3fT2f bottomLeft;
  Vertex3fT2f topRight;
  Vertex3fT2f bottomRight;
};

struct QuadV3fC3fT2f {
  Vertex3fC3fT2f topLeft;
  Vertex3fC3fT2f bottomLeft;
  Vertex3fC3fT2f topRight;
  Vertex3fC3fT2f bottomRight;
};

} // namespace ocf
