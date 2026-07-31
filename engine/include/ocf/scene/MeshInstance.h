// SPDX-License-Identifier: MIT
#pragma once

#include "ocf/core/Reference.h"
#include "ocf/scene/Component.h"

namespace ocf {

class Mesh;

class MeshInstance : public Component {
public:
    MeshInstance();
    ~MeshInstance() override;

    void setMesh(const Ref<Mesh>& mesh);

    Ref<Mesh> getMesh() const noexcept;

private:
    Ref<Mesh> m_mesh;
};

} // namespace ocf
