// SPDX-License-Identifier: MIT

#include "ocf/rhi/Device.h"

namespace ocf {
namespace rhi {

size_t Device::getElementTypeSize(ElementType type)
{
    switch (type) {
    case ElementType::Byte:             return sizeof(int8_t);
    case ElementType::Ubyte:            return sizeof(uint8_t);
    case ElementType::Short:            return sizeof(int16_t);
    case ElementType::Ushort:           return sizeof(uint16_t);
    case ElementType::Int:              return sizeof(int32_t);
    case ElementType::Uint:             return sizeof(uint32_t);
    case ElementType::Float:            return sizeof(float);
    case ElementType::Double:           return sizeof(double);
    default:
        return 0;
    }
}

Device::Device() = default;
Device::~Device() = default;

} // namespace rhi
} // namespace ocf
