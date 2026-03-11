#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

namespace ocf {

// Transform class stores position, rotation (quaternion), and scale.
// Data members are 16-byte aligned to allow future SIMD optimization.
class alignas(16) Transform {
public:
    Transform();

    void setPosition(const glm::vec3& position);
    glm::vec3 getPosition() const;

    void setRotation(const glm::vec3& eulerAngles);
    glm::vec3 getRotation() const;

    void setScale(const glm::vec3& scale);
    glm::vec3 getScale() const;

    // Returns the model matrix composed from position, rotation, and scale.
    glm::mat4 getMatrix() const;

private:
    alignas(16) glm::vec3 m_position;
    alignas(16) glm::quat m_rotation;
    alignas(16) glm::vec3 m_scale;
};

} // namespace ocf
