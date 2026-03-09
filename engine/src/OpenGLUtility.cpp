#include "ocf/OpenGLUtility.h"
#include <csignal>
#include <fstream>
#include <sstream>
#include <iostream>

static bool isCompiled(GLuint shader)
{
    GLint status;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &status);

    if (status != GL_TRUE) {
        char buffer[512] = {0};
        glGetShaderInfoLog(shader, 511, nullptr, buffer);
        std::cerr << "OpenGL Compile Failed: " << buffer << std::endl;
        return false;
    }

    return true;
}

static GLuint compileShader(ocf::ShaderStage stage, std::string_view source)
{
    std::ifstream shaderFile(source.data());
    if (!shaderFile) {
        std::cerr << "Shader file not found: " << source.data() << std::endl;
        return false;
    }

    std::stringstream sstream;
    sstream << shaderFile.rdbuf();
    std::string contents = sstream.str();
    const char* contentChar = contents.c_str();

    GLuint shader = glCreateShader(ocf::OpenGLUtility::getShaderStage(stage));
    glShaderSource(shader, 1, &(contentChar), nullptr);
    glCompileShader(shader);

    if (!isCompiled(shader)) {
        std::cerr << "Failed to compile shader: " << source.data() << std::endl;
        return GL_NONE;
    }

    return shader;
}

static bool isValidProgram(GLuint program)
{
    GLint status;

    glGetProgramiv(program, GL_LINK_STATUS, &status);
    if (status != GL_TRUE) {
        char buffer[512] = {0};
        glGetProgramInfoLog(program, 511, nullptr, buffer);
        std::cerr << "OpenGL Link Failed: " << buffer << std::endl;
        return false;
    }

    return true;
}

namespace ocf {

const char* OpenGLUtility::getGLError(GLenum error) noexcept
{
    const char* string = "unknown";
    switch (error) {
    case GL_NO_ERROR:
        string = "GL_NO_ERROR";
        break;
    case GL_INVALID_ENUM:
        string = "GL_INVALID_ENUM";
        break;
    case GL_INVALID_VALUE:
        string = "GL_INVALID_VALUE";
        break;
    case GL_INVALID_OPERATION:
        string = "GL_INVALID_OPERATION";
        break;
    case GL_INVALID_FRAMEBUFFER_OPERATION:
        string = "GL_INVALID_FRAMEBUFFER_OPERATION";
        break;
    case GL_OUT_OF_MEMORY:
        string = "GL_OUT_OF_MEMORY";
        break;
    default:
        break;
    }
    return string;
}

GLenum OpenGLUtility::checkGLError(std::ostream& out, const char* function, size_t line) noexcept
{
    const GLenum error = glGetError();
    if (error != GL_NO_ERROR) {
        const char* errorString = getGLError(error);
        out << "OpenGL error " << std::hex << error << " (" << errorString << ") in \"" << function
            << "\" at line " << std::dec << line << std::endl;
    }
    return error;
}

void OpenGLUtility::assertGLError(std::ostream& out, const char* function, size_t line) noexcept
{
    const GLenum error = checkGLError(out, function, line);
    if (error != GL_NO_ERROR) {
        std::raise(SIGINT);
    }
}

GLenum OpenGLUtility::toGLPrimitive(PrimitiveType primitiveType)
{
    GLenum result = GL_TRIANGLES;
    switch (primitiveType) {
    case PrimitiveType::POINTS:
        result = GL_POINTS;
        break;
    case PrimitiveType::LINES:
        result = GL_LINES;
        break;
    case PrimitiveType::TRIANGLES:
        result = GL_TRIANGLES;
        break;
    default:
        break;
    }

    return result;
}

//GLenum OpenGLUtility::toGLFormat(PixelFormat format)
//{
//    GLenum result = 0;
//
//    switch (format) {
//    case PixelFormat::GRAY:
//        result = GL_RED;
//        break;
//    case PixelFormat::RGB:
//        result = GL_RGB;
//        break;
//    case PixelFormat::RGBA:
//        result = GL_RGBA;
//        break;
//    default:
//        assert(false);
//        break;
//    }
//
//    return result;
//}

GLsizei OpenGLUtility::getGLDataTypeSize(GLenum type)
{
    GLsizei size = 0;
    switch (type) {
    case GL_BOOL:
    case GL_BYTE:
    case GL_UNSIGNED_BYTE:
        size = sizeof(GLbyte);
        break;
    case GL_BOOL_VEC2:
    case GL_SHORT:
    case GL_UNSIGNED_SHORT:
        size = sizeof(GLshort);
        break;
    case GL_BOOL_VEC3:
        size = sizeof(GLboolean);
        break;
    case GL_BOOL_VEC4:
    case GL_INT:
    case GL_UNSIGNED_INT:
    case GL_FLOAT:
        size = sizeof(GLfloat);
        break;
    case GL_FLOAT_VEC2:
    case GL_INT_VEC2:
        size = sizeof(GLfloat) * 2;
        break;
    case GL_FLOAT_VEC3:
    case GL_INT_VEC3:
        size = sizeof(GLfloat) * 3;
        break;
    case GL_FLOAT_MAT2:
    case GL_FLOAT_VEC4:
    case GL_INT_VEC4:
        size = sizeof(GLfloat) * 4;
        break;
    case GL_FLOAT_MAT3:
        size = sizeof(GLfloat) * 9;
        break;
    case GL_FLOAT_MAT4:
        size = sizeof(GLfloat) * 16;
        break;
    default:
        break;
    }
    return size;
}

GLuint OpenGLUtility::loadShader(ShaderStage stage, std::string_view source)
{
    return compileShader(stage, source);
}

GLuint OpenGLUtility::compileProgram(GLuint vertexShader, GLuint fragmentShader)
{
    if (vertexShader == 0 || fragmentShader == 0) {
        return 0;
    }

    GLuint program = glCreateProgram();

    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);

    glLinkProgram(program);

    if (!isValidProgram(program)) {
        glDeleteProgram(program);
        program = 0;
    }

    return program;
}

} // namespace ocf::backend
