#ifndef LDISHADER_HPP
#define LDISHADER_HPP

// Standard Library
#include <string>
#include <vector>

// OpenGL
#include <GL/glew.h>

enum LDI_SHADER_ENUMS {
    LDI_SHADER_VF,
    LDI_SHADER_VGF,
    LDI_SHADER_C,
    LDI_SHADER_VTCTEF,
    LDI_SHADER_VTCTEGF
};

class LDIShader
{
public:
    LDIShader(const std::vector<std::string> &shaderPaths, LDI_SHADER_ENUMS shaderType);
    ~LDIShader();

    GLuint getProgramID() { return m_program; }
    //void use();

    static std::string shaderDirectory;

private:
    LDIShader();
    GLuint compileKernel(const std::string &shaderPath, GLenum shaderType);

    GLuint m_program;
};

#endif
