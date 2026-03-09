#include "ocf/Program.h"
#include "ocf/OpenGLUtility.h"
#include <glad/gl.h>

namespace ocf {

struct Program::BuilderDetails {
	std::string vertexShaderSource;
    std::string fragmentShaderSource;
};

Program::Builder::Builder()
{
    m_impl = new BuilderDetails();
}

Program::Builder::~Builder()
{
    delete m_impl;
}

Program::Builder& Program::Builder::vertexShader(std::string_view filePath) noexcept
{
    m_impl->vertexShaderSource = filePath;
    return *this;
}

Program::Builder& Program::Builder::fragmentShader(std::string_view filePath) noexcept
{
    m_impl->fragmentShaderSource = filePath;
    return *this;
}

Program* Program::Builder::build()
{
    return new Program(*this);
}

Program::Program(const Builder& builder)
    : m_vertexShaderSource(builder->vertexShaderSource)
    , m_fragmentShaderSource(builder->fragmentShaderSource)
{
    GLuint vertexShader =
        OpenGLUtility::loadShader(ShaderStage::VERTEX, m_vertexShaderSource.c_str());

    GLuint fragmentShader =
        OpenGLUtility::loadShader(ShaderStage::FRAGMENT, m_fragmentShaderSource.c_str());

    m_handle = OpenGLUtility::compileProgram(vertexShader, fragmentShader);

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
}

Program::~Program()
{
    if (m_handle != 0) {
        glDeleteProgram(m_handle);
    }
}

} // namespace ocf