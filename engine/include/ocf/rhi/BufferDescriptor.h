#pragma once

namespace ocf::rhi {

class BufferDescriptor {
public:
    using Callback = void (*)(void* buffer, size_t size, void* user);

    BufferDescriptor() = default;

    ~BufferDescriptor()
    {

    }

    BufferDescriptor(const BufferDescriptor&) = delete;
    BufferDescriptor& operator=(const BufferDescriptor&) = delete;

    BufferDescriptor(BufferDescriptor&& rhs) noexcept
        : buffer(rhs.buffer)
        , size(rhs.size)
        , m_callback(rhs.m_callback)
        , m_user(rhs.m_user)
    {
        rhs.buffer = nullptr;
        rhs.m_callback = nullptr;
    }

    BufferDescriptor& operator=(BufferDescriptor&& rhs) noexcept
    {
        if (this != &rhs) {
            buffer = rhs.buffer;
            size = rhs.size;
            m_callback = rhs.m_callback;
            m_user = rhs.m_user;
            rhs.buffer = nullptr;
            rhs.m_callback = nullptr;
        }
        return *this;
    }

    BufferDescriptor(void* buffer, size_t size, Callback callback = nullptr, void* user = nullptr)
        : buffer(buffer)
        , size(size)
        , m_callback(callback)
        , m_user(user)
    {
    }

    bool hasCallback() const { return m_callback != nullptr; }

    Callback getCallback() const { return m_callback; }

    void* getBuffer() const { return buffer; }

    void* getUser() const { return m_user; }

    void* buffer = nullptr;

    size_t size = 0;

private:
    Callback m_callback = nullptr;
    void* m_user = nullptr;
};

} // namespace ocf::rhi
