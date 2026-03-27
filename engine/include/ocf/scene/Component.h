// SPDX - License - Identifier : MIT

#pragma once
#include "ocf/core/Object.h"

namespace ocf {

class Node;

class OCF_API Component : public Object {
public:
    Component();
    virtual ~Component();

    virtual void update(float deltaTime);

protected:
    Node* m_owner = nullptr;
};

} // namespace ocf
