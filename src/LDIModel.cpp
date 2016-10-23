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
    m_view(),
    m_x_resolution(rx), m_y_resolution(ry),
    m_screenWidth(0), m_screenHeight(0),
    m_shaderFrameBuffer({"ldi_fboPass.vert", "ldi_fboPass.frag"}, LDI_SHADER_VF),
    m_shaderInitPixelHashTable({"initPixelHashTable.glsl"}, LDI_SHADER_C),
    m_shaderFillPixelHashTable({"ldi_fboPass.vert", "fillPixelHashTable.frag"}, LDI_SHADER_VF),
    m_shaderInitPrefixSum({"initPrefixSum.glsl"}, LDI_SHADER_C),
    m_shaderPrefixSum({"prefixSum.glsl"}, LDI_SHADER_C)
{
    m_screenWidth = std::ceil(view.width / m_x_resolution);
    m_screenHeight = std::ceil(view.height / m_y_resolution);
    std::cout << "width: " << m_screenWidth << std::endl;
    std::cout << "height: " << m_screenHeight << std::endl;

    GLint shaderFrameBufferProg = m_shaderFrameBuffer.getProgramID();
    GLint shaderFillPixelHashTableProg = m_shaderFillPixelHashTable.getProgramID();

    ///////////////////////////////////////////////////////////////////////////
    // Creation d'un uniform buffer object
    // (stocke la matrice proj * view * model pour les shaders)
    glGenBuffers(1, &m_ubo);
    glBindBuffer(GL_UNIFORM_BUFFER, m_ubo);
    glBufferData(GL_UNIFORM_BUFFER, sizeof(glm::mat4), NULL, GL_STREAM_COPY);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);

    setOrthogonalView(view);

    // Bind l'uniform buffer object a l'index 2 dans la table de liaison d'OpenGL
    GLuint binding_ubo_point_index = 2;
    glBindBufferBase(GL_UNIFORM_BUFFER, binding_ubo_point_index, m_ubo);

    GLuint block_index = 0;

    // Bind la variable "projection" des shaders a l'index 2 dans la table de liaison d'OpenGL
    glUseProgram(shaderFrameBufferProg);
    block_index = glGetUniformBlockIndex(shaderFrameBufferProg, "projection");
    glUniformBlockBinding(shaderFrameBufferProg, block_index, binding_ubo_point_index);

    glUseProgram(shaderFillPixelHashTableProg);
    block_index = glGetUniformBlockIndex(shaderFillPixelHashTableProg, "projection");
    glUniformBlockBinding(shaderFillPixelHashTableProg, block_index, binding_ubo_point_index);

    glUseProgram(0);

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

    // depth render inutile si c'est juste pour calculer le nombres de pixels rendus
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
    m_view = view;
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

unsigned int LDIModel::getNbPixelFrags()
{
    GLint program = m_shaderFrameBuffer.getProgramID();
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

// initialise et rempli la hash table
void LDIModel::hashPixels(unsigned int nbPixels)
{
}

//void LDIModel::prefixSum()
//{
//}

// Warning, change le viewport d'OpenGL
// Table de liaison OpenGL:
//      0 = nothing
//      1 = nothing
//      2 = nothing
//      3 = ssbo pixelHashTable
//      4 = atomic_counter
//      5 = ssbo_prefixSum
std::vector<pixel_frag> LDIModel::getPixelFrags()
{
    GLint old_program;
    glGetIntegerv(GL_CURRENT_PROGRAM, &old_program);
    GLint old_viewport[4];
    glGetIntegerv(GL_VIEWPORT, old_viewport);

    glDisable(GL_DEPTH_TEST);

    glViewport(0, 0, m_screenWidth, m_screenHeight);
    glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);

    std::vector<pixel_frag> pixelFrags(0);

    unsigned int nbPixels = getNbPixelFrags();
    std::cout << "OpenGL: " << nbPixels << " pixels rasterises" << std::endl;
    if (nbPixels == 0) {
        return std::vector<pixel_frag>(0);
    }

    GLint programInitPixelHashTable = m_shaderInitPixelHashTable.getProgramID();
    GLint programFillPixelHashTable = m_shaderFillPixelHashTable.getProgramID();
    GLint programInitPrefixSum = m_shaderInitPrefixSum.getProgramID();
    GLint programPrefixSum = m_shaderPrefixSum.getProgramID();
    unsigned int maxPixels = m_screenWidth * m_screenHeight;

    /// Build pixelHashTable
    ///////////////////////////////////////////////////////////////////////////
    // Creation d'un shader storage buffer object
    // (initialisation de la table de hashage)
    GLuint ssbo_pixelHashTable;
    glGenBuffers(1, &ssbo_pixelHashTable);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_pixelHashTable);
    glBufferData(GL_SHADER_STORAGE_BUFFER, maxPixels*sizeof(GLuint), NULL, GL_STATIC_DRAW);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

    // Bind le ssbo_pixelHashTable a l'index 3 dans la table de liaison d'OpenGL
    GLuint binding_ssbo1_point_index = 3;
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, binding_ssbo1_point_index, ssbo_pixelHashTable);

    glUseProgram(programInitPixelHashTable);
    GLuint maxPixelsLoc = glGetUniformLocation(programInitPixelHashTable, "max_pixels");
    glUniform1ui(maxPixelsLoc, maxPixels);

    unsigned int dim_x = std::ceil( maxPixels/256.0f ) + 1;
    glDispatchCompute(dim_x, 1, 1);

    ///////////////////////////////////////////////////////////////////////////
    // Creation d'un atomic counter
    // (compte le nombre de passe dans le fragment shader)
    GLuint atomic_counter;
    glGenBuffers(1, &atomic_counter);
    glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, atomic_counter);
    glBufferData(GL_ATOMIC_COUNTER_BUFFER, sizeof(GLuint), 0, GL_STREAM_READ);
    glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, 0);

    // Bind l'ac a l'index 4 dans la table de liaison d'OpenGL
    GLuint binding_ac_point_index = 4;
    glBindBufferBase(GL_ATOMIC_COUNTER_BUFFER, binding_ac_point_index, atomic_counter);

    ///////////////////////////////////////////////////////////////////////////
    // Remplissage du shader storage buffer object
    // (indique le nombre de fragments presents dans chaque pixel)
    glUseProgram(programFillPixelHashTable);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    GLuint screenWidthLoc = glGetUniformLocation(programFillPixelHashTable, "screen_width");
    glUniform1ui(screenWidthLoc, m_screenWidth);

    draw();

    // fetch ssbo_pixelHashTable
    std::vector<unsigned int> v(maxPixels);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_pixelHashTable);
    GLuint* ssbo_ptr = (GLuint*) glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_READ_ONLY);
    std::memcpy(v.data(), ssbo_ptr, maxPixels*sizeof(GLuint));
    glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);

//    // fetch atomic_counter
//    GLuint counter;
//    glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, atomic_counter);
//    GLuint* ac_ptr = (GLuint*) glMapBuffer(GL_ATOMIC_COUNTER_BUFFER, GL_READ_ONLY);
//    std::memcpy(&counter, ac_ptr, sizeof(GLuint));
//    glUnmapBuffer(GL_ATOMIC_COUNTER_BUFFER);

//    unsigned int n = 0;
//    for (unsigned int i = 0; i < v.size(); ++i) {
//        n += v[i];
//    }
//    // n == counter == nbPixels

    /// Build prefixSum
    ///////////////////////////////////////////////////////////////////////////
    // Creation d'un shader storage buffer object
    // (initialisation du prefix sum)
    GLuint ssbo_prefixSum;
    glGenBuffers(1, &ssbo_prefixSum);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_prefixSum);
    glBufferData(GL_SHADER_STORAGE_BUFFER, maxPixels*sizeof(GLuint), NULL, GL_STATIC_DRAW);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

    // Bind le ssbo_prefixSum a l'index 5 dans la table de liaison d'OpenGL
    GLuint bind_ssbo2_point_index = 5;
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, bind_ssbo2_point_index, ssbo_prefixSum);

    // technique du lache
    for (unsigned int i = 1; i < v.size(); ++i) {
        v[i] += v[i-1];
    }
    // v[v.size-1] == counter

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_prefixSum);
    glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, maxPixels*sizeof(GLuint), v.data());
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

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

//    /// Build pixelFragTable
//    ///////////////////////////////////////////////////////////////////////////
//    // Creation d'un shader storage buffer object
//    // (initialisation du prefix sum)
//    GLuint ssbo_pixelFragTable;
//    glGenBuffers(1, &ssbo_pixelFragTable);
//    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_pixelFragTable);
//    glBufferData(GL_SHADER_STORAGE_BUFFER, maxPixels*sizeof(pixel_frag), NULL, GL_STATIC_DRAW);
//    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

    glUseProgram(0);

    //// Render ldi fragment in a ssbo ?


    glDeleteBuffers(1, &atomic_counter);
    glDeleteBuffers(1, &ssbo_prefixSum);
    glDeleteBuffers(1, &ssbo_pixelHashTable);


    // discutable
    glEnable(GL_DEPTH_TEST);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    glUseProgram(old_program);
    glViewport(old_viewport[0], old_viewport[1], old_viewport[2], old_viewport[3]);

    return pixelFrags;

//    GLsizei height_nb = (int)(height/y_resolution);
//    GLsizei width_nb = (int)(width/x_resolution);
//    glGetIntegerv(GL_VIEWPORT, m_viewport);
//    glViewport(0, 0, width_nb, height_nb);
//    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
//    m_screenWidth = width_nb;
//    m_screenHeight = height_nb;
//    bindFBOWA(m_fbo, m_screenWidth, m_screenHeight);
//    //rasterization
//    getNbPixelFrags(camCenter, normal, upDir, height, width, depth);
//    //std::cout<<"getPixelFrags_2: m_nbPixels: "<<m_nbPixels<<std::endl;
//    if(m_nbPixels == 0)
//    {
//        //deleteFBOWA(&m_fbo);
//        debindFBOWA();
//        return m_pixelFrags;
//    }
//    unsigned int estimated_memory_usage = 16*4 + m_nbPixels*8 +
//            m_nbPixels*4 + (m_nbPixels/2048+1)*4 + m_nbPixels*4 + m_nbPixels*64;
//    //std::cout<<"estimated_memory_usage: "<<estimated_memory_usage<<std::endl;
//    if(estimated_memory_usage/1000 > getCurrentAvailableMemory()*0.8)
//        return std::vector<pixel_frag>(0);
//    hashPixels();
//    buildPrefixSums();
//    rasterizationOpt();
//    bubbleSort();
//    //fetch the output
//    getOptFrags();
//    m_pixelFrags.clear();
//    m_pixelFrags.resize(m_optFrags.size());
//    //std::cout<<m_screenHeight<<", "<<m_screenWidth<<std::endl;
//    for(int i=0; i<m_pixelFrags.size(); i++)
//    {
//        GLuint key = m_optFrags[i].info_2[0];
//        GLuint m_i = key/m_screenHeight;
//        GLuint m_j = key-m_i*m_screenHeight;
//        m_pixelFrags[i].m_i = m_i;
//        m_pixelFrags[i].m_j = m_j;
//        m_pixelFrags[i].m_z = m_optFrags[i].info_2[1];
//        m_pixelFrags[i].m_idObj = m_optFrags[i].info_1[0] & 0x3FFFFFFF;
//        //std::cout<<m_i<<", "<<m_j<<", "<<m_pixelFrags[i].m_z<<std::endl;
//        //std::cout<<m_optFrags[i].info_3[0]<<std::endl;
//    }
//    freeOptGradientsBuffers();
//    //deleteFBOWA(&m_fbo);
//    debindFBOWA();
//    resetViewport();
//    return m_pixelFrags;
}

void LDIModel::draw()
{
    for(unsigned int i = 0; i < m_meshes.size(); ++i) {
        m_meshes[i]->draw();
    }
}
