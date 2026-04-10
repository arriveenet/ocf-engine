// SPDX-License-Identifier: MIT

#pragma once
#include "ocf/platform//platform.h"
#include <stdint.h>

namespace ocf {

class OCF_API Object {
public:
    Object();
    Object(uint32_t id)
        : m_id(id)
    {
    }

    virtual ~Object() = default;

    uint32_t getId() const { return m_id; } 

private:
    uint32_t m_id = 0;
};

} // namespace ocf
