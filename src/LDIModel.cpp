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
    m_shaderFillPixelHashTable({"ldi_fboPass.vert", "fillPixelHashTable.frag"}, LDI_SHADER_VF),
    m_shaderPrefixSum({"prefixSum.glsl"}, LDI_SHADER_C),
    m_shaderBlockSum({"blockSum.glsl"}, LDI_SHADER_C),
    m_shaderAddBlockSum({"addBlockSum.glsl"}, LDI_SHADER_C),
    m_shaderPixelFrag({"ldi_fboPass.vert", "fillPixelFrag.frag"}, LDI_SHADER_VF),
    m_shaderFillIndexFrag({"fillIndexFrag.glsl"}, LDI_SHADER_C),
    m_shaderSortPixelFrag({"sortPixelFrag.glsl"}, LDI_SHADER_C)
{
    m_screenWidth = std::ceil(view.width / m_x_resolution);
    m_screenHeight = std::ceil(view.height / m_y_resolution);

    ///////////////////////////////////////////////////////////////////////////
    // Creation d'un uniform buffer object
    // (stocke la matrice proj * view * model pour les shaders)
    glGenBuffers(1, &m_ubo);
    glBindBuffer(GL_UNIFORM_BUFFER, m_ubo);
    glBufferData(GL_UNIFORM_BUFFER, sizeof(glm::mat4), NULL, GL_STATIC_READ);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);

    // Bind l'uniform buffer object a l'index 1 dans la table de liaison d'OpenGL
    GLuint binding_ubo_point_index = 0;
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

void LDIModel::getNbPixelFrag(GLuint &ac_countFrag)
{
    GLuint programCountPixelFrag = m_shaderCountPixelFrag.getProgramID();

    ///////////////////////////////////////////////////////////////////////////
    // Creation d'un atomic counter
    // (initialisation du compteur)
    glGenBuffers(1, &ac_countFrag);
    glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, ac_countFrag);
    glBufferData(GL_ATOMIC_COUNTER_BUFFER, sizeof(GLuint), 0, GL_STREAM_READ);
    glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, 0);

    // Bind l'ac a l'index 1 dans la table de liaison d'OpenGL
    GLuint binding_ac_countFrag_point_index = 1;
    glBindBufferBase(GL_ATOMIC_COUNTER_BUFFER, binding_ac_countFrag_point_index, ac_countFrag);

    ///////////////////////////////////////////////////////////////////////////
    // Incrementatation de l'atomic counter
    // (compte le nombre de passe dans le fragment shader)
    glUseProgram(programCountPixelFrag);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    draw();
}

void LDIModel::hashPixel(GLuint &ac_countPixel, GLuint &ssbo_pixelHashTable, unsigned int maxPixel)
{
    GLuint programFillPixelHashTable = m_shaderFillPixelHashTable.getProgramID();

    GLuint data[maxPixel];
    for (unsigned int i = 0; i < maxPixel; ++i) {
        data[i] = 0;
    }

    /// Build pixelHashTable
    ///////////////////////////////////////////////////////////////////////////
    // Creation d'un atomic counter
    // (initialisation du compteur)
    glGenBuffers(1, &ac_countPixel);
    glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, ac_countPixel);
    glBufferData(GL_ATOMIC_COUNTER_BUFFER, sizeof(GLuint), 0, GL_STREAM_READ);
    glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, 0);

    // Bind l'ac a l'index 2 dans la table de liaison d'OpenGL
    GLuint binding_ac_countPixel_point_index = 2;
    glBindBufferBase(GL_ATOMIC_COUNTER_BUFFER, binding_ac_countPixel_point_index, ac_countPixel);

    ///////////////////////////////////////////////////////////////////////////
    // Creation d'un shader storage buffer object
    // (initialisation de la table de hashage)
    glGenBuffers(1, &ssbo_pixelHashTable);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_pixelHashTable);
    glBufferData(GL_SHADER_STORAGE_BUFFER, maxPixel*sizeof(GLuint), &data, GL_STATIC_READ);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

    // Bind le ssbo_pixelHashTable a l'index 3 dans la table de liaison d'OpenGL
    GLuint binding_ssbo_point_index = 3;
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, binding_ssbo_point_index, ssbo_pixelHashTable);

    ///////////////////////////////////////////////////////////////////////////
    // Remplissage du shader storage buffer object
    // (indique le nombre de fragments presents dans chaque pixel)
    glUseProgram(programFillPixelHashTable);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    GLuint screenWidthLoc = glGetUniformLocation(programFillPixelHashTable, "screen_width");
    glUniform1ui(screenWidthLoc, m_screenWidth);

    draw();
}

void LDIModel::prefixSum(GLuint &ssbo_prefixSum, GLuint &ssbo_blockSum, unsigned int maxPixel)
{
    GLuint programPrefixSum = m_shaderPrefixSum.getProgramID();
    GLuint programBlockSum = m_shaderBlockSum.getProgramID();
    GLuint programAddBlockSum = m_shaderAddBlockSum.getProgramID();

    unsigned int n = std::ceil( maxPixel/2048.0f );
    maxPixel = n * 2048;

    GLuint prefixSum_data[maxPixel];
    for (unsigned int i = 0; i < maxPixel; ++i) {
        prefixSum_data[i] = 0;
    }

    GLuint blockSum_data[n];
    for (unsigned int i = 0; i < n; ++i) {
        blockSum_data[i] = 0;
    }

    /// Build prefixSum and blockSum
    ///////////////////////////////////////////////////////////////////////////
    // Creation d'un shader storage buffer object
    // (initialisation du prefix sum)
    glGenBuffers(1, &ssbo_prefixSum);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_prefixSum);
    glBufferData(GL_SHADER_STORAGE_BUFFER, maxPixel*sizeof(GLuint), &prefixSum_data, GL_STATIC_READ);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

    // Bind le ssbo_prefixSum a l'index 5 dans la table de liaison d'OpenGL
    GLuint bind_ssbo_prefixSum_point_index = 5;
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, bind_ssbo_prefixSum_point_index, ssbo_prefixSum);

    ///////////////////////////////////////////////////////////////////////////
    // Creation d'un shader storage buffer object
    // (initialisation du block sum)
    glGenBuffers(1, &ssbo_blockSum);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_blockSum);
    glBufferData(GL_SHADER_STORAGE_BUFFER, n*sizeof(GLuint), &blockSum_data, GL_STATIC_READ);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

    // Bind le ssbo_blockSum a l'index 6 dans la table de liaison d'OpenGL
    GLuint bind_ssbo_blockSum_point_index = 6;
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, bind_ssbo_blockSum_point_index, ssbo_blockSum);

    ///////////////////////////////////////////////////////////////////////////
    // Remplissage du shader storage buffer object
    // (calcul du prefix sum)
    glUseProgram(programPrefixSum);

    glDispatchCompute(n, 1, 1);

    ///////////////////////////////////////////////////////////////////////////
    // Remplissage du shader storage buffer object
    // (calcul du block sum)
    glUseProgram(programBlockSum);
    GLuint maxBlockLoc = glGetUniformLocation(programBlockSum, "max_block");
    glUniform1ui(maxBlockLoc, n);

    glDispatchCompute(1, 1, 1);

    ///////////////////////////////////////////////////////////////////////////
    // Remplissage du shader storage buffer object
    // (calcul du prefix sum final)
    glUseProgram(programAddBlockSum);

    glDispatchCompute(n, 1, 1);
}

void LDIModel::pixelFrag(GLuint &ssbo_pixelFrag, unsigned int nbPixelFrag)
{
    GLuint programPixelFrag = m_shaderPixelFrag.getProgramID();

    pixel_frag pixelFrag_data[nbPixelFrag];

    /// Build pixelFrag
    ///////////////////////////////////////////////////////////////////////////
    // Creation d'un shader storage buffer object
    // (initialisation du tableau des pixel_frag)
    glGenBuffers(1, &ssbo_pixelFrag);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_pixelFrag);
    glBufferData(GL_SHADER_STORAGE_BUFFER, nbPixelFrag*sizeof(pixel_frag), &pixelFrag_data, GL_STATIC_READ);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

    // Bind le ssbo_pixelFrag a l'index 7 dans la table de liaison d'OpenGL
    GLuint bind_ssbo_pixelFrag_point_index = 7;
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, bind_ssbo_pixelFrag_point_index, ssbo_pixelFrag);

    ///////////////////////////////////////////////////////////////////////////
    // Remplissage du shader storage buffer object
    // (creation des pixel_frag dans la structure de donnees)
    glUseProgram(programPixelFrag);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    GLuint screenWidthLoc = glGetUniformLocation(programPixelFrag, "screen_width");
    glUniform1ui(screenWidthLoc, m_screenWidth);

    draw();
}

void LDIModel::indexFrag(GLuint &ssbo_indexFrag, unsigned int maxPixel, unsigned int nbPixel)
{
    GLuint programFillIndexFrag = m_shaderFillIndexFrag.getProgramID();

    GLuint indexFrag_data[nbPixel];
    for (unsigned int i = 0; i < nbPixel; ++i) {
        indexFrag_data[i] = 0;
    }

    /// Build indexFrag
    ///////////////////////////////////////////////////////////////////////////
    // Creation d'un shader storage buffer object
    // (initialisation du tableau des indices)
    glGenBuffers(1, &ssbo_indexFrag);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_indexFrag);
    glBufferData(GL_SHADER_STORAGE_BUFFER, nbPixel*sizeof(GLuint), &indexFrag_data, GL_STATIC_READ);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

    // Bind le ssbo_indexFrag a l'index 8 dans la table de liaison d'OpenGL
    GLuint bind_ssbo_indexFrag_point_index = 8;
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, bind_ssbo_indexFrag_point_index, ssbo_indexFrag);

    ///////////////////////////////////////////////////////////////////////////
    // Remplissage du shader storage buffer object
    // (calcul les indices des pixels)
    glUseProgram(programFillIndexFrag);

    GLuint maxPixelLoc = glGetUniformLocation(programFillIndexFrag, "max_pixel");
    glUniform1ui(maxPixelLoc, maxPixel);

    glDispatchCompute(1, 1, 1);
}

void LDIModel::sortPixelFrag(unsigned int nbPixel)
{
    GLuint programSortPixelFrag = m_shaderSortPixelFrag.getProgramID();

    /// Sort pixelFrag
    ///////////////////////////////////////////////////////////////////////////
    glUseProgram(programSortPixelFrag);
    GLuint nbPixelLoc = glGetUniformLocation(programSortPixelFrag, "nb_pixel");
    glUniform1ui(nbPixelLoc, nbPixel);

    glDispatchCompute(nbPixel, 1, 1);
}

std::vector<pixel_frag> LDIModel::getPixelFrag()
{
    GLint old_program;
    glGetIntegerv(GL_CURRENT_PROGRAM, &old_program);
    GLint old_viewport[4];
    glGetIntegerv(GL_VIEWPORT, old_viewport);

    glViewport(0, 0, m_screenWidth, m_screenHeight);
    glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);

    unsigned int maxPixel = m_screenWidth * m_screenHeight;

    ///////////////////////////////////////////////////////////////////////////
    /// 1 : perform a full fragment count
    GLuint ac_countFrag;
    getNbPixelFrag(ac_countFrag);

    // fetch ac_countFrag
    unsigned int nbPixelFrag = 0;
    glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, ac_countFrag);
    GLuint* ac_countFrag_ptr = (GLuint*) glMapBuffer(GL_ATOMIC_COUNTER_BUFFER, GL_READ_ONLY);
    std::memcpy(&nbPixelFrag, ac_countFrag_ptr, sizeof(GLuint));
    glUnmapBuffer(GL_ATOMIC_COUNTER_BUFFER);

    std::vector<pixel_frag> vPixelFrag(nbPixelFrag);
    //std::cout << "OpenGL: " << nbPixelFrag << " pixels rasterises" << std::endl;

    if (nbPixelFrag > 0) {
        ///////////////////////////////////////////////////////////////////////////
        /// 1.1 : perform a fragment count on each pixel
        GLuint ac_countPixel;
        GLuint ssbo_pixelHashTable;
        hashPixel(ac_countPixel, ssbo_pixelHashTable, maxPixel);

        // fetch ac_countPixel
        unsigned int nbPixel = 0;
        glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, ac_countPixel);
        GLuint* ac_countPixel_ptr = (GLuint*) glMapBuffer(GL_ATOMIC_COUNTER_BUFFER, GL_READ_ONLY);
        std::memcpy(&nbPixel, ac_countPixel_ptr, sizeof(GLuint));
        glUnmapBuffer(GL_ATOMIC_COUNTER_BUFFER);

        ///////////////////////////////////////////////////////////////////////////
        /// 1.2 : perform a prefixSum on the hash table
        GLuint ssbo_prefixSum;
        GLuint ssbo_blockSum;
        prefixSum(ssbo_prefixSum, ssbo_blockSum, maxPixel);

        ///////////////////////////////////////////////////////////////////////////
        /// 1.3 : fill each pixel with fragments
        GLuint ssbo_pixelFrag;
        pixelFrag(ssbo_pixelFrag, nbPixelFrag);

        ///////////////////////////////////////////////////////////////////////////
        /// 1.4 : indexing each pixel
        GLuint ssbo_indexFrag;
        indexFrag(ssbo_indexFrag, maxPixel, nbPixel);

        ///////////////////////////////////////////////////////////////////////////
        /// 1.5 : sort fragments according to their depth
        sortPixelFrag(nbPixel);

        // fetch ssbo_pixelFrag
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_pixelFrag);
        GLuint* ssbo_pixelFrag_ptr = (GLuint*) glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_READ_ONLY);
        std::memcpy(vPixelFrag.data(), ssbo_pixelFrag_ptr, nbPixelFrag*sizeof(pixel_frag));
        glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);

        // // debug
        // for (unsigned int i = 0; i < vPixelFrag.size(); ++i) {
        //     pixel_frag &p = vPixelFrag[i];
        //     std::cout << p.m_i << ", " << p.m_j << " : " << p.m_z << std::endl;
        // }

        glDeleteBuffers(1, &ac_countPixel);
        glDeleteBuffers(1, &ssbo_pixelHashTable);
        glDeleteBuffers(1, &ssbo_prefixSum);
        glDeleteBuffers(1, &ssbo_blockSum);
        glDeleteBuffers(1, &ssbo_pixelFrag);
        glDeleteBuffers(1, &ssbo_indexFrag);
    }

    glDeleteBuffers(1, &ac_countFrag);

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
