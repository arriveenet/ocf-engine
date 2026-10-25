// SPDX-License-Identifier: MIT
#pragma once
#include<memory>

namespace ocf::rhi {

template <typename T>
class GpuResourceBase {
public:
    // Copy is prohibited
    GpuResourceBase(const GpuResourceBase&) = delete;
    GpuResourceBase& operator=(const GpuResourceBase&) = delete;

    virtual ~GpuResourceBase() = default;

    static std::shared_ptr<T> create() { return std::shared_ptr<T>(new T()); }

protected:
    GpuResourceBase() = default;
};

} // namespace ocf::rhi
