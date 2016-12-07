
// Standard Library
#include <iostream>
#include <string>
#include <vector>
#include <thread>

// OpenGL
#include <GL/glew.h>
#include <GL/gl.h>

// GLFW
//#include <GLFW/glfw3.h>

// Glut
#include <GL/freeglut.h>

// GLM
#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

// Project
#include "LDIMesh.hpp"
#include "LDIModel.hpp"
#include "LDIShader.hpp"

#ifdef CMAKE_CWD
    #define CWD CMAKE_CWD
#else
    #define CWD "Error"
#endif

//const int framerate = 20;
const GLuint screenWidth = 640;
const GLuint screenHeight = 480;
//GLFWwindow* window;

GLuint vaoQuad, quad, quadTex;
void initVaoQuad();
void destroyVaoQuad();

GLuint frameBuffer;
GLuint renderColor;
GLuint renderNormal;
GLuint renderDepth;
void initFrameBuffer(unsigned int width, unsigned int height);
void destroyFrameBuffer();

void sendVariablesToShader(GLuint program);
void attachTextureToShader(GLuint program);

std::vector<LDIMesh*> vLDIMesh;
GLuint shaderProg;
GLuint shaderFrameBufferProg;

LDIShader *shader;
LDIShader *shaderFrameBuffer;

void init() {
    ///////////////////////////////////////////////////////////////////////////
    // Magic !
    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) {
        std::cerr << "Failed to initialize GLEW" << std::endl;
        throw std::exception();
    }

    ///////////////////////////////////////////////////////////////////////////
    // Initialisation de la machine a etat OpenGL
    glClearColor(0.0, 1.0, 0.0, 0.0);
    glEnable(GL_DEPTH_TEST);

    ///////////////////////////////////////////////////////////////////////////
    // Creation des meshes (vao, vbo, ...)
    std::string objFilename1 = CWD + std::string("/models/dragon_low.obj");
    //std::string objFilename1 = CWD + std::string("/models/cube.obj");
    LDIMesh *ldiMesh1 = LDIMesh::fromObj(objFilename1);
    vLDIMesh.push_back(ldiMesh1);

    // Creation du LDIModel
    LDIModel::orthoView view;
    // Cube
    // view.camCenter = glm::vec3(0,0,2);
    // view.normalDir = glm::vec3(0,0,-1);
    // view.upDir = glm::vec3(0,1,0);
    // view.width = 2;
    // view.height = 2;
    // view.depth = 4;
    // LDIModel ldiModel(vLDIMesh, view, 1.0f, 1.0f);
    // Dragon
    view.camCenter = glm::vec3(0,0,6);
    view.normalDir = glm::vec3(0,0,-1);
    view.upDir = glm::vec3(0,1,0);
    view.width = 20;
    view.height = 20;
    view.depth = 20;
    LDIModel ldiModel(vLDIMesh, view, 0.1f, 0.1f);

    std::vector<LDIModel::pixel_frag> pixelFrags = ldiModel.getPixelFrag();
    std::cout << "info: " << pixelFrags.size() << " fragments recuperes" << std::endl;
    //int nb = ldiModel.getNbPixelFrags();
    //std::cout << nb << " pixels rendus dans le fbo" << std::endl;
    //glViewport(0, 0, width, height);

    ///////////////////////////////////////////////////////////////////////////
    // Creation du vaoQuad pour afficher le frameBuffer
    initVaoQuad();

    ///////////////////////////////////////////////////////////////////////////
    // Creation d'un frameBuffer et de ses textures
    initFrameBuffer(screenWidth, screenHeight);

    ///////////////////////////////////////////////////////////////////////////
    // Creation des shaders
    std::vector<std::string> shaderPath;
    shaderPath.push_back("basic.vert");
    shaderPath.push_back("basic.frag");
    shader = new LDIShader(shaderPath, LDI_SHADER_VF);

    std::vector<std::string> shaderPathFrameBuffer;
    shaderPathFrameBuffer.push_back("fboPass.vert");
    shaderPathFrameBuffer.push_back("fboPass.frag");
    shaderFrameBuffer = new LDIShader(shaderPathFrameBuffer, LDI_SHADER_VF);

    shaderProg = shader->getProgramID();
    shaderFrameBufferProg = shaderFrameBuffer->getProgramID();

    ///////////////////////////////////////////////////////////////////////////
    // Passer les matrices au premier shader ici
    sendVariablesToShader(shaderFrameBufferProg);

    ///////////////////////////////////////////////////////////////////////////
    // Passer les textures au second shader ici
    attachTextureToShader(shaderProg);
}

void afficher() {
    ///////////////////////////////////////////////////////////////////////////
    // Dessiner dans le framebuffer (premiere passe)
    glBindFramebuffer(GL_FRAMEBUFFER, frameBuffer);

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glUseProgram(shaderFrameBufferProg);

    for (unsigned int i = 0; i < vLDIMesh.size(); ++i) {
        LDIMesh *mesh = vLDIMesh[i];
        mesh->draw();
    }

    // Dessiner dans le framebuffer d'OpenGL (seconde passe)
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glUseProgram(shaderProg);

    glBindVertexArray(vaoQuad);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);

    ///////////////////////////////////////////////////////////////////////////
    // Swap le front et le back buffer
    glutSwapBuffers();
    std::cout << "display" << std::endl;
}

void refenetrer(int width, int height) {
    glViewport(0, 0, (GLsizei) width, (GLsizei) height);
}

void clavier(unsigned char touche, int x, int y)
{
    switch (touche)
    {
        case 27:    // Escape
            //glutLeaveMainLoop();
            glutPostRedisplay();
            break;
    }
}

void gerer_souris(int bouton, int etat, int x, int y) {

}

void gerer_souris_mouvement(int x, int y) {

}

void terminate() {
    for (unsigned int i = 0; i < vLDIMesh.size(); ++i) {
        LDIMesh::destroy(vLDIMesh[i]);
    }
    delete shader;
    delete shaderFrameBuffer;
    destroyFrameBuffer();
    destroyVaoQuad();
    glUseProgram(0);
}

int main(int argvc, char **argv)
{
    try {
        glutInit(&argvc, argv);
        glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);    /* affichage couleur */
        glutInitWindowSize(screenWidth, screenHeight);  /* taille initiale fenetre graphique */
        glutInitWindowPosition(200,200);                /* position initiale */
        glutCreateWindow(argv[0]);                      /* creation de la fenetre graphique et du contexte OpenGL */

        init();

        glutDisplayFunc(afficher);                      /* fonction d’affichage */
        glutReshapeFunc(refenetrer);                    /* fonction de refenetrage */
        glutKeyboardFunc(clavier);                      /* gestion du clavier */
        glutMouseFunc(gerer_souris);                    /* fonction souris  */
        glutMotionFunc(gerer_souris_mouvement);         /* deplacement de la souris */
        glutMainLoop();                                 /* lancement de la boucle principale */

        terminate();
    } catch (const std::exception &) {
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;

/*
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
    glClearColor(0.0, 1.0, 0.0, 0.0);
    glEnable(GL_DEPTH_TEST);

    ///////////////////////////////////////////////////////////////////////////
    // Creation des meshes (vao, vbo, ...)
    std::vector<LDIMesh*> vLDIMesh;
    std::string objFilename1 = CWD + std::string("/models/dragon_low.obj");
    //std::string objFilename1 = CWD + std::string("/models/cube.obj");
    LDIMesh *ldiMesh1 = LDIMesh::fromObj(objFilename1);
    vLDIMesh.push_back(ldiMesh1);

    // Creation du LDIModel
    LDIModel::orthoView view;
    // Cube
    // view.camCenter = glm::vec3(0,0,2);
    // view.normalDir = glm::vec3(0,0,-1);
    // view.upDir = glm::vec3(0,1,0);
    // view.width = 2;
    // view.height = 2;
    // view.depth = 4;
    // LDIModel ldiModel(vLDIMesh, view, 1.0f, 1.0f);
    // Dragon
    view.camCenter = glm::vec3(0,0,6);
    view.normalDir = glm::vec3(0,0,-1);
    view.upDir = glm::vec3(0,1,0);
    view.width = 20;
    view.height = 20;
    view.depth = 20;
    LDIModel ldiModel(vLDIMesh, view, 0.1f, 0.1f);

    std::vector<LDIModel::pixel_frag> pixelFrags = ldiModel.getPixelFrag();
    std::cout << "info: " << pixelFrags.size() << " fragments recuperes" << std::endl;
    //int nb = ldiModel.getNbPixelFrags();
    //std::cout << nb << " pixels rendus dans le fbo" << std::endl;
    //glViewport(0, 0, width, height);

    ///////////////////////////////////////////////////////////////////////////
    // Creation du vaoQuad pour afficher le frameBuffer
    initVaoQuad();

    ///////////////////////////////////////////////////////////////////////////
    // Creation d'un frameBuffer et de ses textures
    initFrameBuffer(width, height);

    ///////////////////////////////////////////////////////////////////////////
    // Creation des shaders
    std::vector<std::string> shaderPath;
    shaderPath.push_back("basic.vert");
    shaderPath.push_back("basic.frag");
    LDIShader shader(shaderPath, LDI_SHADER_VF);

    std::vector<std::string> shaderPathFrameBuffer;
    shaderPathFrameBuffer.push_back("fboPass.vert");
    shaderPathFrameBuffer.push_back("fboPass.frag");
    LDIShader shaderFrameBuffer(shaderPathFrameBuffer, LDI_SHADER_VF);

    GLuint shaderProg = shader.getProgramID();
    GLuint shaderFrameBufferProg = shaderFrameBuffer.getProgramID();

    ///////////////////////////////////////////////////////////////////////////
    // Passer les matrices au premier shader ici
    sendVariablesToShader(shaderFrameBufferProg);

    ///////////////////////////////////////////////////////////////////////////
    // Passer les textures au second shader ici
    attachTextureToShader(shaderProg);

    ///////////////////////////////////////////////////////////////////////////
    // Dessiner dans le framebuffer (premiere passe)
    glBindFramebuffer(GL_FRAMEBUFFER, frameBuffer);

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glUseProgram(shaderFrameBufferProg);

    for (unsigned int i = 0; i < vLDIMesh.size(); ++i) {
        LDIMesh *mesh = vLDIMesh[i];
        mesh->draw();
    }

    // Dessiner dans le framebuffer d'OpenGL (seconde passe)
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glUseProgram(shaderProg);

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

    destroyFrameBuffer();

    destroyVaoQuad();

    glUseProgram(0);

    glfwDestroyWindow(window);
    glfwTerminate();

    return EXIT_SUCCESS;
*/
}

void initVaoQuad()
{
    const GLfloat quadData[] = {
      -1.0f, -1.0f, 0.0f,
       1.0f, -1.0f, 0.0f,
      -1.0f,  1.0f, 0.0f,
      -1.0f,  1.0f, 0.0f,
       1.0f, -1.0f, 0.0f,
       1.0f,  1.0f, 0.0f,
    };

    const GLfloat quadDataTex[] = {
        0.0f, 0.0f,
        1.0f, 0.0f,
        0.0f, 1.0f,
        0.0f, 1.0f,
        1.0f, 0.0f,
        1.0f, 1.0f,
    };

    glGenVertexArrays(1, &vaoQuad);
    glGenBuffers(1, &quad);
    glGenBuffers(1, &quadTex);

    glBindVertexArray(vaoQuad);

    glBindBuffer(GL_ARRAY_BUFFER, quad);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadData), quadData, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*) 0);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, quadTex);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadDataTex), quadDataTex, GL_STATIC_DRAW);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, (void*) 0);
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);
}

void destroyVaoQuad()
{
    glDeleteBuffers(1, &quad);
    glDeleteBuffers(1, &quadTex);
    glDeleteVertexArrays(1, &vaoQuad);
}

void initFrameBuffer(unsigned int width, unsigned int height)
{
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

    GLuint tab[] = {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1};
    glDrawBuffers(2, tab);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void destroyFrameBuffer()
{
    glDeleteTextures(1, &renderColor);
    glDeleteTextures(1, &renderNormal);
    glDeleteTextures(1, &renderDepth);
    glDeleteFramebuffers(1, &frameBuffer);
}

void sendVariablesToShader(GLuint program)
{
    glUseProgram(program);

    glm::vec3 camCenter(0,0,10);
    glm::vec3 lookAt(0,0,0);
    glm::vec3 upDir(0,1,0);
    float depth = 20.0f;

    glm::mat4 projMat_persp = glm::perspective(90.0f, (float)screenWidth / (float)screenHeight, 0.1f, 40.0f);
    glm::mat4 projMat_ortho = glm::ortho(-10.0f, 10.0f, -10.0f, 10.0f, 0.0f, depth);
    glm::mat4 viewMat = glm::lookAt(camCenter, lookAt, upDir);
    glm::mat4 modelMat = glm::mat4();
    glm::vec3 light = glm::vec3(0,0,-1);

    GLuint projLoc = glGetUniformLocation(program, "projMat_persp");
    GLuint projLoc_ortho = glGetUniformLocation(program, "projMat_ortho");
    GLuint viewLoc = glGetUniformLocation(program, "viewMat");
    GLuint modelLoc = glGetUniformLocation(program, "modelMat");
    GLuint lightLoc = glGetUniformLocation(program, "light");

    glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projMat_persp));
    glUniformMatrix4fv(projLoc_ortho, 1, GL_FALSE, glm::value_ptr(projMat_ortho));
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(viewMat));
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(modelMat));
    glUniform3fv(lightLoc, 1, glm::value_ptr(light));

    glUseProgram(0);
}

void attachTextureToShader(GLuint program)
{
    glUseProgram(program);

    // color texture
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, renderColor);
    glUniform1i(glGetUniformLocation(program, "textureColor"), 0);

    // normal texture
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, renderNormal);
    glUniform1i(glGetUniformLocation(program, "textureNormal"), 1);

    // depth texture
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, renderDepth);
    glUniform1i(glGetUniformLocation(program, "textureDepth"), 2);

    glUseProgram(0);
}
