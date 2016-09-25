#ifndef UTILS_HPP
#define UTILS_HPP

#include <GL/glew.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <vector>
#include "../include/LDIstructs.hpp"
#include <ctime>
#include <iostream>
#include <SOIL/SOIL.h>
#include <cstdarg>

#define GL_GPU_MEM_INFO_TOTAL_AVAILABLE_MEM_NVX 0x9048
#define GL_GPU_MEM_INFO_CURRENT_AVAILABLE_MEM_NVX 0x9049

template<typename T>
static void initSSBO(GLuint *ssbo, size_t size, GLint binding)
{
    glGenBuffers(1, ssbo);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, *ssbo);
    glBufferData(GL_SHADER_STORAGE_BUFFER, size*sizeof(T), NULL, GL_STATIC_DRAW);
    //glBufferStorage(GL_SHADER_STORAGE_BUFFER, size*sizeof(T), NULL, GL_MAP_READ_BIT );
    //glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, binding, *ssbo);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}

template<typename T>
static void fetchSSBO(GLuint ssbo, size_t size, std::vector<T> & v)
{
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo);
    //on récupère les pixel_frags
    T *ssbo_ptr = (T*) glMapBufferRange(
                                   GL_SHADER_STORAGE_BUFFER, 0, size*sizeof(T),
                                   GL_MAP_READ_BIT);
    v.resize(size);
    std::memcpy(v.data(), ssbo_ptr, size*sizeof(T));
    glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}

static void initSSBO_offset(GLuint *ssbo, std::vector<uint32_t> & v, GLint binding)
{
    glGenBuffers(1, ssbo);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, *ssbo);
    glBufferData(GL_SHADER_STORAGE_BUFFER, v.size()*sizeof(uint32_t), v.data(),
                GL_STATIC_DRAW);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, binding, *ssbo);
}

static void initTexture(GLuint *texture, std::string filepath)
{
    glGenTextures(1, texture);
    glBindTexture(GL_TEXTURE_2D, *texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    int width, height;
    unsigned char* image = SOIL_load_image(filepath.c_str(), &width, &height, 0, SOIL_LOAD_RGB);
    //std::cout<<"width: "<<width<<", height: "<<height<<std::endl;
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height,0, GL_RGB, GL_UNSIGNED_BYTE, image);
    glGenerateMipmap(GL_TEXTURE_2D);
    SOIL_free_image_data(image);
    //const char * res = SOIL_last_result();
    //std::cout<<"SOIL_last_result: "<<res<<std::endl;
    glBindTexture(GL_TEXTURE_2D, 0);
}

static void bindTexture(GLuint texture)
{
    glBindTexture(GL_TEXTURE_2D, texture);
}

static void bindSSBO(GLuint ssbo, GLint binding)
{
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, binding, ssbo);
}

static void initABO(GLuint *abo, GLint binding)
{
    GLuint initABO = 0;
    glGenBuffers(1, abo);
    glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, *abo);
    glBufferData(GL_ATOMIC_COUNTER_BUFFER, sizeof(GLuint), &initABO,
                GL_STATIC_DRAW);
    glBindBufferBase(GL_ATOMIC_COUNTER_BUFFER, binding, *abo);
}

static void bindABO(GLuint abo, GLint binding)
{
    glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, abo);
    glBindBufferBase(GL_ATOMIC_COUNTER_BUFFER, binding, abo);
}

static unsigned int getABO(GLuint abo)
{
    glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, 0);
    glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, abo);
    unsigned int *readCounter = (unsigned int*)glMapBufferRange
            (GL_ATOMIC_COUNTER_BUFFER, 0, sizeof(unsigned int),
             GL_MAP_READ_BIT);
    unsigned int val = readCounter[0];
    glUnmapBuffer(GL_ATOMIC_COUNTER_BUFFER);
    glFinish();
    return val;
}

static unsigned int getABO_named(GLuint abo)
{
    unsigned int *readCounter = (unsigned int*)glMapNamedBufferRange
            (abo, 0, sizeof(unsigned int),
             GL_MAP_READ_BIT);
    unsigned int val = readCounter[0];
    glUnmapNamedBuffer(abo);
    return val;
}

static void printVec3(glm::vec3 v)
{
    std::cout<<v[0]<<", "<<v[1]<<", "<<v[2]<<std::endl;
}

static void printMat4(glm::mat4 m)
{
    std::cout<<m[0][0]<<", "<<m[0][1]<<", "<<m[0][2]<<", "<<m[0][3]<<std::endl;
    std::cout<<m[1][0]<<", "<<m[1][1]<<", "<<m[1][2]<<", "<<m[1][3]<<std::endl;
    std::cout<<m[2][0]<<", "<<m[2][1]<<", "<<m[2][2]<<", "<<m[2][3]<<std::endl;
    std::cout<<m[3][0]<<", "<<m[3][1]<<", "<<m[3][2]<<", "<<m[3][3]<<std::endl;
}

static glm::mat4 getViewMat(box viewBox)
{
    //we need the normal vector and the quadcenter
    glm::vec3 center = (viewBox.a+viewBox.b+viewBox.c+viewBox.d)/4.0f;
    glm::vec3 normal = glm::normalize(glm::cross(viewBox.d-viewBox.a, viewBox.b-viewBox.a));
    glm::vec3 upDir = glm::normalize(viewBox.d-viewBox.a);
    return glm::lookAt(center, center+normal, upDir);
}

static void initOrthogonalOptView(box viewBox, glm::mat4 &projMat, glm::mat4 &viewMat,
                                  glm::mat4 & modelMat)
{
    float width = glm::length(viewBox.b-viewBox.a);
    float height = glm::length(viewBox.d-viewBox.a);
    glm::mat4 proj = glm::ortho(-width/2, width/2, -height/2, height/2, 0.0f, viewBox.depth);
    glm::mat4 view = getViewMat(viewBox);
    glm::mat4 model;
    projMat = glm::mat4(proj);
    viewMat = glm::mat4(view);
    modelMat = glm::mat4(model);
}

static void initOrthogonalOptFragsView(glm::mat4 &projMat, glm::mat4 &viewMat, glm::mat4 &modelMat,
                                       glm::vec3 camCenter, glm::vec3 normal, glm::vec3 upDir, float height, float width,
                                       float depth)
{
    projMat = glm::ortho(-width/2, width/2, -height/2, height/2, 0.0f, depth);
    viewMat = glm::lookAt(camCenter, camCenter+normal, upDir);
    modelMat = glm::mat4();
}

static void bindOrthogonalOptView(GLint shaderProg, glm::mat4 &projMat, glm::mat4 & viewMat,
                                  glm::mat4 & modelMat)
{
    int projLoc = glGetUniformLocation(shaderProg,
                                               "projMat");
    int viewLoc = glGetUniformLocation(shaderProg,
                                               "viewMat");
    int modelLoc = glGetUniformLocation(shaderProg,
                                                "modelMat");

    glUniformMatrix4fv(projLoc, 1, GL_FALSE, (glm::value_ptr(projMat)));
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, (glm::value_ptr(viewMat)));
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, (glm::value_ptr(modelMat)));
}

static void initOrthogonalView(GLint shaderProg, box viewBox)
{
    int projLoc = glGetUniformLocation(shaderProg,
                                               "projMat");
    int viewLoc = glGetUniformLocation(shaderProg,
                                               "viewMat");
    int modelLoc = glGetUniformLocation(shaderProg,
                                                "modelMat");
    //on utilise une projection orthogonale pour des calculs GPGPU
    float width = glm::length(viewBox.b-viewBox.a);
    float height = glm::length(viewBox.d-viewBox.a);
    glm::mat4 proj = glm::ortho(-width/2, width/2, -height/2, height/2, 0.0f, viewBox.depth);
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(proj));
    glm::mat4 view = getViewMat(viewBox);
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE,
                               glm::value_ptr(view));
    //glm::mat4 model = glm::scale(glm::mat4(), glm::vec3(0.08));
    glm::mat4 model;
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
}

static void bindFBOWA(GLuint fbo, GLsizei width, GLsizei height)
{
    //FBO without attachement
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glFramebufferParameteri(GL_FRAMEBUFFER,
                                    GL_FRAMEBUFFER_DEFAULT_WIDTH, width);
    glFramebufferParameteri(GL_FRAMEBUFFER,
                                    GL_FRAMEBUFFER_DEFAULT_HEIGHT, height);
}

static void initFBOWA(GLuint *fbo, GLsizei width, GLsizei height)
{
    //FBO without attachement
    glGenFramebuffers(1, fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, *fbo);
    glFramebufferParameteri(GL_FRAMEBUFFER,
                                    GL_FRAMEBUFFER_DEFAULT_WIDTH, width);
    glFramebufferParameteri(GL_FRAMEBUFFER,
                                    GL_FRAMEBUFFER_DEFAULT_HEIGHT, height);
}

static void debindFBOWA()
{
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

static void deleteFBOWA(GLuint *fbo)
{
    glDeleteFramebuffers(1, fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

static void printBinary(uint n)
{
    for(int i=0; i<32; i++)
    {
        std::cout<<n%2;
        n -= n%2;
        n /= 2;
    }
    std::cout<<std::endl;
}

static GLint getTotalAvailableMemory()
{
    // to figure out the available video memory, we have to use either nvidia or ati
    // extensions
    GLint res = 0;
    glGetIntegerv(GL_VBO_FREE_MEMORY_ATI, &res);
    if(res != 0)
        return res;
    GLint nTotalMemoryInKB = 0;
    glGetIntegerv( GL_GPU_MEM_INFO_TOTAL_AVAILABLE_MEM_NVX,
                   &nTotalMemoryInKB );

    return nTotalMemoryInKB;
}

static GLint getCurrentAvailableMemory()
{
    // to figure out the available video memory, we have to use either nvidia or ati
    // extensions
    GLint res = 0;
    glGetIntegerv(GL_VBO_FREE_MEMORY_ATI, &res);
    if(res != 0)
        return res;

    GLint nCurAvailMemoryInKB = 0;
    glGetIntegerv( GL_GPU_MEM_INFO_CURRENT_AVAILABLE_MEM_NVX,
                   &nCurAvailMemoryInKB );
    return nCurAvailMemoryInKB;
}

#endif
