// SPDX-License-Identifier: MIT

#include "ocf/core/Object.h"

namespace ocf {

Object::Object()
{
    static uint32_t nextId = 1; // Start IDs from 1
    m_id = nextId++;
}

} // namespace ocf
