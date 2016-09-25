
// Standard Library
#include <iostream>
#include <string>
#include <vector>
#include <thread>

// OpenGL
#include <GL/glew.h>
#include <GL/gl.h>

// GLFW
#include <GLFW/glfw3.h>

// GLM
#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

// Project
#include "LDIMesh.hpp"
#include "LDIShader.hpp"

#ifdef CMAKE_CWD
    #define CWD CMAKE_CWD
#else
    #define CWD "Error"
#endif

const int framerate = 20;
const GLuint screenWidth = 2*320;
const GLuint screenHeight = 2*240;
GLFWwindow* window;

int main() {

    if (!glfwInit()) {
        std::cerr << "Failed to init glfw context" << std::endl;
        return EXIT_FAILURE;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);

    GLFWwindow *window = glfwCreateWindow(screenWidth, screenHeight, "OpenGL", NULL, NULL);
    if (!window) {
        glfwTerminate();
        return EXIT_FAILURE;
    }

    glfwMakeContextCurrent(window);

    glewExperimental = true;
    if (glewInit() != GLEW_OK) {
        std::cerr << "Failed to initialize GLEW" << std::endl;
        return EXIT_FAILURE;
    }

    int width, height;
    glfwGetFramebufferSize(window, &width, &height);
    glViewport(0, 0, width, height);

    //glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glClearColor(0.0, 0.0, 0.0, 0.0);
    glEnable(GL_DEPTH_TEST);

    ///////////////////////////////////////////////////////////////////////////
    // Creation des meshes (vao, vbo, ...)
    std::vector<LDIMesh*> vLDIMesh;
    std::string objFilename1 = CWD + std::string("/models/dragon_low.obj");
    LDIMesh *ldiMesh1 = LDIMesh::fromObj(objFilename1);
    vLDIMesh.push_back(ldiMesh1);

    // Creation du vaoQuad pour le frameBuffer
    GLuint vaoQuad;
    GLuint quad;
    const GLfloat quadData[] = {
      -1.0f, -1.0f, 0.0f,
       1.0f, -1.0f, 0.0f,
      -1.0f,  1.0f, 0.0f,
      -1.0f,  1.0f, 0.0f,
       1.0f, -1.0f, 0.0f,
       1.0f,  1.0f, 0.0f,
    };

    glGenVertexArrays(1, &vaoQuad);
    glGenBuffers(1, &quad);

    glBindVertexArray(vaoQuad);

    glBindBuffer(GL_ARRAY_BUFFER, quad);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadData), quadData, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*) 0);
    glEnableVertexAttribArray(0);

    glBindVertexArray(0);

    ///////////////////////////////////////////////////////////////////////////
    // Creation d'un frameBuffer et de ses textures
    GLuint frameBuffer;
    GLuint renderColor;
    GLuint renderNormal;
    GLuint renderDepth;

    glGenFramebuffers(1, &frameBuffer);
    glGenTextures(1, &renderColor);
    glGenTextures(1, &renderNormal);
    glGenTextures(1, &renderDepth);

    glBindFramebuffer(GL_FRAMEBUFFER, frameBuffer);

    glBindTexture(GL_TEXTURE_2D, renderColor);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, width, height, 0, GL_RGBA, GL_FLOAT, (void*) 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, renderColor, 0);

    glBindTexture(GL_TEXTURE_2D, renderNormal);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, width, height, 0, GL_RGBA, GL_FLOAT, (void*) 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, renderNormal, 0);

    glBindTexture(GL_TEXTURE_2D, renderDepth);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, width, height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, (void*) 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, renderDepth, 0);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    ///////////////////////////////////////////////////////////////////////////
    // Creation des shaders
    std::vector<std::string> shaderPath;
    shaderPath.push_back("vPass.glsl");
    shaderPath.push_back("fPass.glsl");
    LDIShader shader(shaderPath, LDI_SHADER_VF);

    std::vector<std::string> shaderPathFrameBuffer;
    shaderPathFrameBuffer.push_back("vFrameBufferPass.glsl");
    shaderPathFrameBuffer.push_back("fFrameBufferPass.glsl");
    LDIShader shaderFrameBuffer(shaderPathFrameBuffer, LDI_SHADER_VF);

    GLuint shaderProg = shader.getProgramID();
    GLuint shaderFrameBufferProg = shaderFrameBuffer.getProgramID();

    glUseProgram(shaderFrameBufferProg);

    ///////////////////////////////////////////////////////////////////////////
    // Passer les matrices ici
    glm::vec3 camCenter(0,0,6);
    glm::vec3 lookAt(0,0,0);
    glm::vec3 upDir(0,1,0);
    float depth = 20;

    //glm::mat4 projMat = glm::perspective(90.0f, (float)width / (float)height, 0.1f, 40.0f);
    glm::mat4 projMat = glm::ortho(-10.0f, 10.0f, -10.0f, 10.0f, 1.0f, depth);
    glm::mat4 viewMat = glm::lookAt(camCenter, lookAt, upDir);
    glm::mat4 modelMat = glm::mat4();
    glm::vec3 light = glm::vec3(0,0,-1);

    GLuint projLoc = glGetUniformLocation(shaderFrameBufferProg, "projMat");
    GLuint viewLoc = glGetUniformLocation(shaderFrameBufferProg, "viewMat");
    GLuint modelLoc = glGetUniformLocation(shaderFrameBufferProg, "modelMat");
    GLuint lightLoc = glGetUniformLocation(shaderFrameBufferProg, "light");

    glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projMat));
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(viewMat));
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(modelMat));
    glUniform3fv(lightLoc, 1, glm::value_ptr(light));

    ///////////////////////////////////////////////////////////////////////////
    // Dessiner dans le framebuffer (premiere passe)

    glBindFramebuffer(GL_FRAMEBUFFER, frameBuffer);

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glUseProgram(shaderFrameBufferProg);

    for (unsigned int i = 0; i < vLDIMesh.size(); ++i) {
        LDIMesh *mesh = vLDIMesh[i];
        mesh->draw();
    }

    // Dessiner de le framebuffer d'OpenGL (seconde passe)
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glUseProgram(shaderProg);

    // Envoyer les textures ici
    // color texture
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, renderColor);
    glUniform1i(glGetUniformLocation(shaderProg, "textureColor"), 0);

    // normal texture
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, renderNormal);
    glUniform1i(glGetUniformLocation(shaderProg, "textureNormal"), 1);

    // depth texture
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, renderDepth);
    glUniform1i(glGetUniformLocation(shaderProg, "textureDepth"), 2);

    glBindVertexArray(vaoQuad);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);

    ///////////////////////////////////////////////////////////////////////////
    // Boucle evenementielle de GLFW
    do {
        // Swap buffers
        glfwSwapBuffers(window);
        glfwWaitEvents();

        std::this_thread::sleep_for(std::chrono::milliseconds(int(1000 / framerate)));
    } // Check if the ESC key was pressed or the window was closed
    while( glfwGetKey(window, GLFW_KEY_ESCAPE) != GLFW_PRESS && glfwWindowShouldClose(window) == 0 );

    for (unsigned int i = 0; i < vLDIMesh.size(); ++i) {
        LDIMesh::destroy( vLDIMesh[i] );
    }

    glDeleteTextures(1, &renderColor);
    glDeleteTextures(1, &renderNormal);
    glDeleteTextures(1, &renderDepth);
    glDeleteFramebuffers(1, &frameBuffer);

    glDeleteBuffers(1, &quad);
    glDeleteVertexArrays(1, &vaoQuad);

    glUseProgram(0);

    glfwDestroyWindow(window);
    glfwTerminate();

    return EXIT_SUCCESS;
}
