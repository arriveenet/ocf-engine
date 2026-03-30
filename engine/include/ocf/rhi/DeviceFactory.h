// SPDX - License - Identifier : MIT

#pragma once

#include <memory>

namespace ocf {
namespace rhi {

class Device;

class DeviceFactory {
public:
    static DeviceFactory& getInstance();

    virtual ~DeviceFactory() = default;

    virtual std::unique_ptr<Device> create() = 0;

protected:
    DeviceFactory() = default;

private:
    // Non-copyable and non-movable
    DeviceFactory(const DeviceFactory&) = delete;
    DeviceFactory& operator=(const DeviceFactory&) = delete;

    static std::unique_ptr<DeviceFactory> CreateNativeFactory();

};


} // namespace rhi
} // namespace ocf
