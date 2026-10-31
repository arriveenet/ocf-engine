// SPDX-License-Identifier: MIT

#pragma once
#include "ocf/core/Object.h"
#include "ocf/scene/Renderable.h"

#include <vector>

namespace ocf {

class Node;

class OCF_API Component : public Object {
public:
    Component();
    virtual ~Component();

    virtual void update(float deltaTime);

    const std::vector<Renderable*>& getRenderables() const noexcept { return m_renderables; }

    /** @brief The node this component was added to. Not set yet in the constructor. */
    Node* getOwner() const noexcept { return m_owner; }

protected:
    friend class Node;

    Node* m_owner = nullptr;
    std::vector<Renderable*> m_renderables;
};

} // namespace ocf
