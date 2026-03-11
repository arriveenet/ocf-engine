#include "ocf/Transform.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

namespace ocf {

Transform::Transform()
    : m_position(0.0f, 0.0f, 0.0f)
    , m_rotation()
    , m_scale(1.0f, 1.0f, 1.0f)
{}

void Transform::setPosition(const glm::vec3& position)
{
    m_position = position;
}

glm::vec3 Transform::getPosition() const
{
    return m_position;
}

void Transform::setRotation(const glm::vec3& eulerAngles)
{
    m_rotation = glm::quat(eulerAngles);
}

glm::vec3 Transform::getRotation() const
{
    return glm::eulerAngles(m_rotation);
}

void Transform::setScale(const glm::vec3& scale)
{
    m_scale = scale;
}

glm::vec3 Transform::getScale() const
{
    return m_scale;
}

glm::mat4 Transform::getMatrix() const
{
    glm::mat4 mat = glm::mat4(1.0f);
    mat = glm::translate(mat, m_position);
    mat *= glm::mat4_cast(m_rotation);
    mat = glm::scale(mat, m_scale);
    return mat;
}

} // namespace ocf
