// SPDX-License-Identifier: MIT
#pragma once

// SPDX-License-Identifier: MIT

#include "ocf/rhi/Handle.h"

#include <cstdint>
#include <unordered_map>

namespace ocf::rhi {

class HandleAllocator {
public:


private:
    uint32_t m_handleIndex{0};
    std::unordered_map<uint32_t, void*> m_handleMap;

};

} // namespace ocf::rhi
