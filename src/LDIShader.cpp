#include "LDIShader.hpp"

// Standard Library
#include <fstream>
#include <iostream>

#ifdef CMAKE_CWD
   #define LDIOpenGL_DIRECTORY CMAKE_CWD
#else
   #define LDIOpenGL_DIRECTORY ""
#endif

std::string LDIShader::shaderDirectory = LDIOpenGL_DIRECTORY + std::string("/shaders/");

LDIShader::LDIShader(const std::vector<std::string> &shaderPath, LDI_SHADER_ENUMS shaderType) :
    m_program(0)
{
    std::vector<GLuint> vKernels;
    switch(shaderType)
    {
    case LDI_SHADER_C:
        vKernels.push_back(compileKernel(shaderPath[0], GL_COMPUTE_SHADER));
        break;
    case LDI_SHADER_VF:
        vKernels.push_back(compileKernel(shaderPath[0], GL_VERTEX_SHADER));
        vKernels.push_back(compileKernel(shaderPath[1], GL_FRAGMENT_SHADER));
        break;
    case LDI_SHADER_VGF:
        vKernels.push_back(compileKernel(shaderPath[0], GL_VERTEX_SHADER));
        vKernels.push_back(compileKernel(shaderPath[1], GL_GEOMETRY_SHADER));
        vKernels.push_back(compileKernel(shaderPath[2], GL_FRAGMENT_SHADER));
        break;
    case LDI_SHADER_VTCTEF:
        vKernels.push_back(compileKernel(shaderPath[0], GL_VERTEX_SHADER));
        vKernels.push_back(compileKernel(shaderPath[1], GL_TESS_CONTROL_SHADER));
        vKernels.push_back(compileKernel(shaderPath[2], GL_TESS_EVALUATION_SHADER));
        vKernels.push_back(compileKernel(shaderPath[3], GL_FRAGMENT_SHADER));
        break;
    case LDI_SHADER_VTCTEGF:
        vKernels.push_back(compileKernel(shaderPath[0], GL_VERTEX_SHADER));
        vKernels.push_back(compileKernel(shaderPath[1], GL_TESS_CONTROL_SHADER));
        vKernels.push_back(compileKernel(shaderPath[2], GL_TESS_EVALUATION_SHADER));
        vKernels.push_back(compileKernel(shaderPath[3], GL_GEOMETRY_SHADER));
        vKernels.push_back(compileKernel(shaderPath[4], GL_FRAGMENT_SHADER));
    }

    // creation du programme
    m_program = glCreateProgram();
    for (unsigned int i = 0; i < vKernels.size(); ++i) {
        glAttachShader(m_program, vKernels[i]);
    }
    glLinkProgram(m_program);

    // verification de la liaison avec les shaders
    GLint success;
    glGetProgramiv(m_program, GL_LINK_STATUS, &success);
    if (!success)
    {
        GLchar infoLog[512];
        glGetProgramInfoLog(m_program, 512, NULL, infoLog);
        std::cout << "Erreur lors de la liaison du programme : " << infoLog << std::endl;
    }

    // destruction des shaders crees
    for (unsigned int i = 0; i < vKernels.size(); ++i) {
        glDetachShader(m_program, vKernels[i]);
        glDeleteShader(vKernels[i]);
    }
}

LDIShader::~LDIShader() {
    glDeleteProgram(m_program);
}

LDIShader::LDIShader() :
    m_program(0)
{
}

GLuint LDIShader::compileKernel(const std::string &shaderPath, GLenum shaderType)
{
    std::string shaderCode;
    std::ifstream shaderFile(LDIShader::shaderDirectory + shaderPath, std::ios::in);

    if(shaderFile.is_open()) {
        std::string Line = "";
        while(getline(shaderFile, Line)) {
            shaderCode += "\n" + Line;
        }
        shaderFile.close();
    } else {
        std::cout << "Erreur l'ors de l'ouverture du fichier : " << shaderPath << std::endl;
        getchar();
        return 0;
    }
    const GLchar* constShaderCode = shaderCode.c_str();

    GLuint shader;
    GLint success;

    shader = glCreateShader(shaderType);
    glShaderSource(shader, 1, &constShaderCode, NULL);
    glCompileShader(shader);

    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        GLchar infoLog[512];
        glGetShaderInfoLog(shader, 512, NULL, infoLog);
        std::cout << "Erreur lors de la compilation du shader : " << shaderPath << " : " << infoLog << std::endl;
    }

    return shader;
}
