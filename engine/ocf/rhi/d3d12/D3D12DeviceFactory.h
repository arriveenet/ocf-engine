// SPDX-License-Identifier: MIT
#pragma once

#include "ocf/rhi/DeviceFactory.h"

namespace ocf {
namespace rhi {

class D3D12DeviceFactory : public DeviceFactory {
public:
    std::unique_ptr<Device> create() override;
};

} // namespace rhi
} // namespace ocf
