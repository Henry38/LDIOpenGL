#include "LDIModel.hpp"

// Standard Library
#include <cstring>
#include <iostream>

// Glm
#include <glm/gtc/type_ptr.hpp>

LDIModel::LDIModel(const std::vector<LDIMesh*> &vLDIMeshes, const orthoView &view, float rx, float ry) :
    m_meshes(vLDIMeshes),
    m_ubo(0),
    m_fbo(0), m_renderColor(0), m_renderDepth(0),
    m_x_resolution(rx), m_y_resolution(ry),
    m_screenWidth(0), m_screenHeight(0),
    m_shaderFrameBuffer({"ldi_fboPass.vert", "ldi_fboPass.frag"}, LDI_SHADER_VF),
    m_shaderCountPixelFrag({"ldi_fboPass.vert", "countPixelFrag.frag"}, LDI_SHADER_VF),
    m_shaderInitPixelHashTable({"initPixelHashTable.glsl"}, LDI_SHADER_C),
    m_shaderFillPixelHashTable({"ldi_fboPass.vert", "fillPixelHashTable.frag"}, LDI_SHADER_VF),
    m_shaderInitPrefixSum({"initPrefixSum.glsl"}, LDI_SHADER_C),
    m_shaderPrefixSum({"prefixSum.glsl"}, LDI_SHADER_C),
    m_shaderInitPixelFrag({"initPixelFrag.glsl"}, LDI_SHADER_C),
    m_shaderPixelFrag({"ldi_fboPass.vert", "fillPixelFrag.frag"}, LDI_SHADER_VF)
{
    m_screenWidth = std::ceil(view.width / m_x_resolution);
    m_screenHeight = std::ceil(view.height / m_y_resolution);

    ///////////////////////////////////////////////////////////////////////////
    // Creation d'un uniform buffer object
    // (stocke la matrice proj * view * model pour les shaders)
    glGenBuffers(1, &m_ubo);
    glBindBuffer(GL_UNIFORM_BUFFER, m_ubo);
    glBufferData(GL_UNIFORM_BUFFER, sizeof(glm::mat4), NULL, GL_STATIC_DRAW);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);

    // Bind l'uniform buffer object a l'index 1 dans la table de liaison d'OpenGL
    GLuint binding_ubo_point_index = 1;
    glBindBufferBase(GL_UNIFORM_BUFFER, binding_ubo_point_index, m_ubo);

    setOrthogonalView(view);

    ///////////////////////////////////////////////////////////////////////////
    // Creation d'un frame buffer object et de ses textures
    // (rend la scene dans une texture a la resolution choisie)
    glGenFramebuffers(1, &m_fbo);
    glGenTextures(1, &m_renderColor);
    glGenTextures(1, &m_renderDepth);

    glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);

    glBindTexture(GL_TEXTURE_2D, m_renderColor);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, m_screenWidth, m_screenHeight, 0, GL_RGBA, GL_FLOAT, (void*) 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_renderColor, 0);

    glBindTexture(GL_TEXTURE_2D, m_renderDepth);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, m_screenWidth, m_screenHeight, 0, GL_DEPTH_COMPONENT, GL_FLOAT, (void*) 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, m_renderDepth, 0);

    GLuint tab[] = {GL_COLOR_ATTACHMENT0};
    glDrawBuffers(1, tab);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

LDIModel::~LDIModel()
{
    glDeleteTextures(1, &m_renderColor);
    glDeleteTextures(1, &m_renderDepth);
    glDeleteFramebuffers(1, &m_fbo);
    glDeleteBuffers(1, &m_ubo);
}

void LDIModel::setOrthogonalView(const LDIModel::orthoView &view)
{
    float width = view.width;
    float height = view.height;
    float depth = view.depth;
    glm::mat4 projMat = glm::ortho(-width/2, width/2, -height/2, height/2, 0.0f, depth);
    glm::mat4 viewMat = glm::lookAt(view.camCenter, view.camCenter + view.normalDir, view.upDir);
    glm::mat4 modelMat = glm::mat4();
    glm::mat4 projViewModelMat = projMat * viewMat * modelMat;

    // update UBO data
    glBindBuffer(GL_UNIFORM_BUFFER, m_ubo);
    glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(glm::mat4), glm::value_ptr(projViewModelMat));
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
}

unsigned int LDIModel::getPixelPassed() {
    GLuint program = m_shaderFrameBuffer.getProgramID();
    glUseProgram(program);

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    GLuint nbPixelsQuery;
    int nbPixels = 0;

    glGenQueries(1, &nbPixelsQuery);
    glBeginQuery(GL_SAMPLES_PASSED, nbPixelsQuery);
    draw();
    glEndQuery(GL_SAMPLES_PASSED);
    glGetQueryObjectiv(nbPixelsQuery, GL_QUERY_RESULT, &nbPixels);
    glDeleteQueries(1, &nbPixelsQuery);

    return nbPixels;
}

void LDIModel::getNbPixelFrag(GLuint &atomic_counter)
{
    GLuint programCountPixelFrag = m_shaderCountPixelFrag.getProgramID();

    ///////////////////////////////////////////////////////////////////////////
    // Creation d'un atomic counter
    // (initialisation du compteur)
    glGenBuffers(1, &atomic_counter);
    glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, atomic_counter);
    glBufferData(GL_ATOMIC_COUNTER_BUFFER, sizeof(GLuint), 0, GL_STREAM_READ);
    glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, 0);

    // Bind l'ac a l'index 2 dans la table de liaison d'OpenGL
    GLuint binding_ac_point_index = 2;
    glBindBufferBase(GL_ATOMIC_COUNTER_BUFFER, binding_ac_point_index, atomic_counter);

    ///////////////////////////////////////////////////////////////////////////
    // Incrementatation de l'atomic counter
    // (compte le nombre de passe dans le fragment shader)
    glUseProgram(programCountPixelFrag);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    draw();
}

void LDIModel::hashPixel(GLuint &ssbo_pixelHashTable, unsigned int maxPixel)
{
    GLuint programInitPixelHashTable = m_shaderInitPixelHashTable.getProgramID();
    GLuint programFillPixelHashTable = m_shaderFillPixelHashTable.getProgramID();

    /// Build pixelHashTable
    ///////////////////////////////////////////////////////////////////////////
    // Creation d'un shader storage buffer object
    // (initialisation de la table de hashage)
    glGenBuffers(1, &ssbo_pixelHashTable);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_pixelHashTable);
    glBufferData(GL_SHADER_STORAGE_BUFFER, maxPixel*sizeof(GLuint), NULL, GL_STATIC_DRAW);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

    // Bind le ssbo_pixelHashTable a l'index 3 dans la table de liaison d'OpenGL
    GLuint binding_ssbo_point_index = 3;
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, binding_ssbo_point_index, ssbo_pixelHashTable);

    glUseProgram(programInitPixelHashTable);
    GLuint maxPixelsLoc = glGetUniformLocation(programInitPixelHashTable, "max_pixels");
    glUniform1ui(maxPixelsLoc, maxPixel);

    unsigned int dim_x = std::ceil( maxPixel/256.0f );
    glDispatchCompute(dim_x, 1, 1);

    ///////////////////////////////////////////////////////////////////////////
    // Remplissage du shader storage buffer object
    // (indique le nombre de fragments presents dans chaque pixel)
    glUseProgram(programFillPixelHashTable);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    GLuint screenWidthLoc = glGetUniformLocation(programFillPixelHashTable, "screen_width");
    glUniform1ui(screenWidthLoc, m_screenWidth);

    draw();
}

void LDIModel::prefixSum(GLuint &ssbo_prefixSum, unsigned int maxPixel)
{
    //GLuint programInitPrefixSum = m_shaderInitPrefixSum.getProgramID();
    //GLuint programPrefixSum = m_shaderPrefixSum.getProgramID();

    /// Build prefixSum
    ///////////////////////////////////////////////////////////////////////////
    // Creation d'un shader storage buffer object
    // (initialisation du prefix sum)
    glGenBuffers(1, &ssbo_prefixSum);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_prefixSum);
    glBufferData(GL_SHADER_STORAGE_BUFFER, maxPixel*sizeof(GLuint), NULL, GL_STATIC_DRAW);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

    // Bind le ssbo_prefixSum a l'index 5 dans la table de liaison d'OpenGL
    GLuint bind_ssbo_point_index = 5;
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, bind_ssbo_point_index, ssbo_prefixSum);

//    glUseProgram(programInitPrefixSum);
//    maxPixelsLoc = glGetUniformLocation(programInitPixelHashTable, "max_pixels");
//    glUniform1ui(maxPixelsLoc, maxPixels);

//    dim_x = std::ceil( maxPixels/256.0f ) + 1;
//    glDispatchCompute(dim_x, 1, 1);

//    ///////////////////////////////////////////////////////////////////////////
//    // Remplissage du shader storage buffer object
//    // (calcul du prefix sum)
//    glUseProgram(programPrefixSum);
//    maxPixelsLoc = glGetUniformLocation(programPrefixSum, "max_pixels");
//    glUniform1ui(maxPixelsLoc, maxPixels);

//    dim_x = std::ceil( maxPixels/1024.0f ) + 1;
    //    glDispatchCompute(dim_x, 1, 1);
}

void LDIModel::pixelFrag(GLuint &ssbo_pixelFrag, unsigned int nbPixelFrag)
{
    GLuint programInitPixelFrag = m_shaderInitPixelFrag.getProgramID();
    GLuint programPixelFrag = m_shaderPixelFrag.getProgramID();

    /// Build pixelFrag
    ///////////////////////////////////////////////////////////////////////////
    // Creation d'un shader storage buffer object
    // (initialisation du tableau des pixel_frag)
    glGenBuffers(1, &ssbo_pixelFrag);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_pixelFrag);
    glBufferData(GL_SHADER_STORAGE_BUFFER, nbPixelFrag*sizeof(pixel_frag), NULL, GL_DYNAMIC_COPY);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

    // Bind le ssbo_pixelFrag a l'index 6 dans la table de liaison d'OpenGL
    GLuint bind_ssbo3_point_index = 6;
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, bind_ssbo3_point_index, ssbo_pixelFrag);

    glUseProgram(programInitPixelFrag);
    GLuint maxPixelFragLoc = glGetUniformLocation(programInitPixelFrag, "max_pixelFrag");
    glUniform1ui(maxPixelFragLoc, nbPixelFrag);

    unsigned int dim_x = std::ceil( nbPixelFrag/256.0f ) + 1;
    glDispatchCompute(dim_x, 1, 1);

    ///////////////////////////////////////////////////////////////////////////
    // Remplissage du shader storage buffer object
    // (creation des pixel_frag dans la strucutre de donnees)
    glUseProgram(programPixelFrag);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    GLuint screenWidthLoc = glGetUniformLocation(programPixelFrag, "screen_width");
    glUniform1ui(screenWidthLoc, m_screenWidth);

    draw();
}

// Warning, change le viewport d'OpenGL
// Table de liaison OpenGL:
//      0 = nothing
//      1 = m_ubo
//      2 = atomic_counter
//      3 = ssbo pixelHashTable
//      5 = ssbo_prefixSum
//      6 = ssbo_pixelFrag
std::vector<pixel_frag> LDIModel::getPixelFrags()
{
    GLint old_program;
    glGetIntegerv(GL_CURRENT_PROGRAM, &old_program);
    GLint old_viewport[4];
    glGetIntegerv(GL_VIEWPORT, old_viewport);

    glViewport(0, 0, m_screenWidth, m_screenHeight);
    glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);

    unsigned int maxPixel = m_screenWidth * m_screenHeight;

    ///////////////////////////////////////////////////////////////////////////
    GLuint atomic_counter;
    getNbPixelFrag(atomic_counter);

    // fetch atomic_counter
    unsigned int nbPixelFrag = 0;
    glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, atomic_counter);
    GLuint* ac_ptr = (GLuint*) glMapBuffer(GL_ATOMIC_COUNTER_BUFFER, GL_READ_ONLY);
    std::memcpy(&nbPixelFrag, ac_ptr, sizeof(GLuint));
    glUnmapBuffer(GL_ATOMIC_COUNTER_BUFFER);

    std::cout << "OpenGL: " << nbPixelFrag << " pixels rasterises" << std::endl;
    if (nbPixelFrag == 0) {
        return std::vector<pixel_frag>(0);
    }

    ///////////////////////////////////////////////////////////////////////////
    GLuint ssbo_pixelHashTable;
    hashPixel(ssbo_pixelHashTable, maxPixel);

    // fetch ssbo_pixelHashTable
    std::vector<unsigned int> pixelHashTable(maxPixel);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_pixelHashTable);
    GLuint* ssbo_pixelHashTable_ptr = (GLuint*) glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_READ_ONLY);
    std::memcpy(pixelHashTable.data(), ssbo_pixelHashTable_ptr, maxPixel*sizeof(GLuint));
    glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);

    ///////////////////////////////////////////////////////////////////////////
    GLuint ssbo_prefixSum;
    prefixSum(ssbo_prefixSum, maxPixel);

    std::vector<unsigned int> vSum(maxPixel);
    // technique du lache
    // TODO : implementation sur GPU
    vSum[0] = 0;
    for (unsigned int i = 1; i < vSum.size(); ++i) {
        vSum[i] = vSum[i-1] + pixelHashTable[i-1];
    }

    // update internal ssbo_prefixSum data
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_prefixSum);
    glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, maxPixel*sizeof(GLuint), vSum.data());
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

    ///////////////////////////////////////////////////////////////////////////
    GLuint ssbo_pixelFrag;
    pixelFrag(ssbo_pixelFrag, nbPixelFrag);

    // fetch ssbo_pixelFrag
    std::vector<pixel_frag> vPixelFrag(nbPixelFrag);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_pixelFrag);
    GLuint* ssbo_pixelFrag_ptr = (GLuint*) glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_READ_ONLY);
    std::memcpy(vPixelFrag.data(), ssbo_pixelFrag_ptr, nbPixelFrag*sizeof(pixel_frag));
    glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);

    // debug
    for (unsigned int i = 0; i < vPixelFrag.size(); ++i) {
        pixel_frag &p = vPixelFrag[i];
        std::cout << p.m_i << ", " << p.m_j << " : " << p.m_z << std::endl;
    }

    glDeleteBuffers(1, &atomic_counter);
    glDeleteBuffers(1, &ssbo_pixelFrag);
    glDeleteBuffers(1, &ssbo_prefixSum);
    glDeleteBuffers(1, &ssbo_pixelHashTable);

    // discutable
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    glUseProgram(old_program);
    glViewport(old_viewport[0], old_viewport[1], old_viewport[2], old_viewport[3]);

    return vPixelFrag;
}

void LDIModel::draw()
{
    for(unsigned int i = 0; i < m_meshes.size(); ++i) {
        m_meshes[i]->draw();
    }
}
