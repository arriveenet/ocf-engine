// SPDX-License-Identifier: MIT

#include "ocf/rhi/Device.h"

namespace ocf {
namespace rhi {

size_t Device::getElementTypeSize(ElementType type)
{
    switch (type) {
    case ElementType::BYTE:             return sizeof(int8_t);
    case ElementType::UNSIGNED_BYTE:    return sizeof(uint8_t);
    case ElementType::SHORT:            return sizeof(int16_t);
    case ElementType::UNSIGNED_SHORT:   return sizeof(uint16_t);
    case ElementType::INT:              return sizeof(int32_t);
    case ElementType::UNSIGNED_INT:     return sizeof(uint32_t);
    case ElementType::FLOAT:            return sizeof(float);
    case ElementType::DOUBLE:           return sizeof(double);
    default:
        return 0;
    }
}

Device::Device() = default;
Device::~Device() = default;

} // namespace rhi
} // namespace ocf
