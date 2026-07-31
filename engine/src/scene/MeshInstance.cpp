// SPDX-License-Identifier: MIT
#include "ocf/scene/MeshInstance.h"

#include "ocf/resource/Mesh.h"

namespace ocf {

MeshInstance::MeshInstance()
{
}

MeshInstance::~MeshInstance()
{
}

void MeshInstance::setMesh(const Ref<Mesh>& mesh)
{
    if (m_mesh == mesh) {
        return;
    }

    m_mesh = mesh;
}

Ref<Mesh> MeshInstance::getMesh() const noexcept
{
    return m_mesh;
}

} // namespace ocf
