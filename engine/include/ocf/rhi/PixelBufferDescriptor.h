#pragma once
#include "BufferDescriptor.h"
#include "RHIEnums.h"

namespace ocf::rhi {

class PixelBufferDescriptor : public BufferDescriptor {
public:
    using PixelDataFormat = rhi::PixelDataFormat;
    using PixelDataType = rhi::PixelDataType;

    PixelBufferDescriptor() = default;

    PixelBufferDescriptor(void* buffer, size_t size, PixelDataFormat format, PixelDataType type,
                          uint8_t alignment, uint32_t left, uint32_t top, uint32_t stride,
                          Callback callback, void* user)
        : BufferDescriptor(buffer, size, callback, user)
        , left(left)
        , top(top)
        , stride(stride)
        , format(format)
        , type(type)
        , alignment(alignment)
    {
    }

    //PixelBufferDescriptor(void* buffer, size_t size, PixelDataFormat format, PixelDataType type,
    //                      uint8_t alignment = 1, uint32_t left = 0, uint32_t top = 0,
    //                      uint32_t stride = 0, Callback callback = nullptr, void* user = nullptr)
    //    : BufferDescriptor(buffer, size)
    //    , left(left)
    //    , top(top)
    //    , stride(stride)
    //    , format(format)
    //    , type(type)
    //    , alignment(alignment)
    //{
    //}

    PixelBufferDescriptor(void* buffer, size_t size, PixelDataFormat format, PixelDataType type,
                          Callback callback, void* user = nullptr)
        : BufferDescriptor(buffer, size, callback, user)
        , stride(0)
        , format(format)
        , type(type)
        , alignment(1)
    {
    }

    uint32_t left = 0;
    uint32_t top = 0;
    uint32_t stride;
    PixelDataFormat format;
    PixelDataType type : 4;
    uint8_t alignment : 4;
};

} // namespace ocf::rhi
