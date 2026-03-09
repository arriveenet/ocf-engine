#pragma once
#include <string>
namespace ocf {

class Program {
    struct BuilderDetails;

public:
    class Builder {
    public:
        Builder();
        ~Builder();

        BuilderDetails* operator->() noexcept { return m_impl; }
        const BuilderDetails* operator->() const noexcept { return m_impl; }

        Builder& vertexShader(std::string_view filePath) noexcept;
        Builder& fragmentShader(std::string_view filePath) noexcept;

        Program* build();

    private:
        BuilderDetails* m_impl = nullptr;
    };

    Program(const Builder& builder);
    ~Program();

    unsigned int getHandle() const noexcept { return m_handle; }

protected:
    unsigned int m_handle = 0;
    std::string m_vertexShaderSource;
    std::string m_fragmentShaderSource;
};

} // namespace ocf
