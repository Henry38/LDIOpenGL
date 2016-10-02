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
    m_fboPass({"ldi_fboPass.vert", "ldi_fboPass.frag"}, LDI_SHADER_VF)
{
    unsigned int screenWidth = std::ceil(view.width / m_x_resolution);
    unsigned int screenHeight = std::ceil(view.height / m_y_resolution);
    m_screenWidth = screenWidth;
    m_screenHeight = screenHeight;

    GLuint shaderFrameBufferProg = m_fboPass.getProgramID();

    ///////////////////////////////////////////////////////////////////////////
    // Creation d'un uniform buffer object
    glGenBuffers(1, &m_ubo);
    glBindBuffer(GL_UNIFORM_BUFFER, m_ubo);
    glBufferData(GL_UNIFORM_BUFFER, sizeof(glm::mat4), NULL, GL_STREAM_COPY);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);

    setOrthogonalView(view);

    glUseProgram(shaderFrameBufferProg);

    GLuint block_index = glGetUniformBlockIndex(shaderFrameBufferProg, "shader_data");
    GLuint binding_point_index = 2;
    glBindBufferBase(GL_UNIFORM_BUFFER, binding_point_index, m_ubo);
    glUniformBlockBinding(shaderFrameBufferProg, block_index, binding_point_index);

    glUseProgram(0);

    ///////////////////////////////////////////////////////////////////////////
    // Creation d'un frameBuffer et de ses textures
    glGenFramebuffers(1, &m_fbo);
    glGenTextures(1, &m_renderColor);
    glGenTextures(1, &m_renderDepth);

    glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);

    glBindTexture(GL_TEXTURE_2D, m_renderColor);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, screenWidth, screenHeight, 0, GL_RGBA, GL_FLOAT, (void*) 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_renderColor, 0);

    glBindTexture(GL_TEXTURE_2D, m_renderDepth);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, screenWidth, screenHeight, 0, GL_DEPTH_COMPONENT, GL_FLOAT, (void*) 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, m_renderDepth, 0);

    GLuint tab[] = {GL_COLOR_ATTACHMENT0};
    glDrawBuffers(1, tab);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);





    //set global offset for all LDImeshes
    // ???
//    int offset = 0;
//    for(int i=0; i<m_meshes.size(); i++)
//    {
//        m_meshes[i]->initializeOffsets(offset);
//        m_meshes[i]->initializeBuffers();
//    }
//    //on a besoin du nombre de sommets composant tous les models
//    m_totalNbVertex = 0;
//    for(unsigned int i=0; i<m_meshes.size(); i++)
//        m_totalNbVertex += m_meshes[i]->getNbVertices();
//    initializeGradientsData();

    //shader initializations
    //m_shaderPath = LDIShader::m_shaderPath;

    // = initFBOWA(&m_fbo, m_screenWidth, m_screenHeight);

//    std::cout << GL_MAX_FRAMEBUFFER_WIDTH << std::endl;
//    std::cout << GL_MAX_FRAMEBUFFER_HEIGHT << std::endl;

//    LDIShader::addInclude(m_shaderPath + "include/structures.hglsl");
//    LDIShader::addInclude(m_shaderPath + "include/buffers.hglsl");
//    LDIShader::addInclude(m_shaderPath + "include/extensions.hglsl");
//    LDIShader::addInclude(m_shaderPath + "include/uniforms.hglsl");
//    LDIShader::addInclude(m_shaderPath + "include/auxiliary_functions.hglsl");
//    LDIShader::addInclude(m_shaderPath + "include/tessellationEvaluation.hglsl");

//    m_indirectShader = LDIShader({m_shaderPath+"cIndirect.glsl"}, LDI_SHADER_C);
//    m_passShader = LDIShader({m_shaderPath+"vPass.glsl", m_shaderPath+"fPass.glsl"}, LDI_SHADER_VF);
//    m_pixelHashShader = LDIShader({m_shaderPath+"vPass.glsl", m_shaderPath+"fHashPixels.glsl"}, LDI_SHADER_VF);
//    m_zeroInitializationUint64Shader = LDIShader({m_shaderPath+"cZeroInitializationUint64.glsl"}, LDI_SHADER_C);
//    m_zeroInitializationUintShader = LDIShader({m_shaderPath+"cZeroInitializationUint.glsl"}, LDI_SHADER_C);
//    m_zeroInitializationFloatShader = LDIShader({m_shaderPath+"cZeroInitializationFloat.glsl"}, LDI_SHADER_C);
//    m_prefixSumsShader = LDIShader({m_shaderPath+"cSamScanUint64.glsl"}, LDI_SHADER_C);
//    m_blocSumsShader = LDIShader({m_shaderPath+"cBlocSum.glsl"}, LDI_SHADER_C);
//    m_addBlocSumsShader = LDIShader({m_shaderPath+"cAddBlocSum.glsl"}, LDI_SHADER_C);
//    m_rasterizationOptShader = LDIShader({m_shaderPath+"vRasterizationOpt.glsl", m_shaderPath+"gRasterizationOpt.glsl",
//                                                         m_shaderPath+"fRasterizationOpt.glsl"}, LDI_SHADER_VGF);
//    m_bubbleSortShader = LDIShader({m_shaderPath+"cBubbleSort.glsl"}, LDI_SHADER_C);
//    m_gradientsOptShader = LDIShader({m_shaderPath+"cGradientsOpt.glsl"}, LDI_SHADER_C);
//    //MULTI_GRADIENT Shaders
//    m_sparseGradientsShader = LDIShader({m_shaderPath+"cSparseGradients.glsl"}, LDI_SHADER_C);
//    m_boxComputeOptShader = LDIShader({m_shaderPath+"cBoxOpt.glsl"}, LDI_SHADER_C);
//    m_multiGradientsOptShader = LDIShader({m_shaderPath+"cMultiGradientsOpt.glsl"}, LDI_SHADER_C);
//    m_sparseMultiGradientsShader = LDIShader({m_shaderPath+"cSparseMultiGradients.glsl"}, LDI_SHADER_C);
//    m_getIntervalsShader = LDIShader({m_shaderPath+"cGetIntervals.glsl"}, LDI_SHADER_C);
//    m_getVoxelFragsShader = LDIShader({m_shaderPath+"cGetVoxelFrags.glsl"}, LDI_SHADER_C);
//    m_getVoxelFragsDistribShader = LDIShader({m_shaderPath+"cGetVoxelFragsDistrib.glsl"}, LDI_SHADER_C);
//    //RADIX_SORT Shaders
//    m_transposeShader = LDIShader({m_shaderPath+"cTranspose.glsl"}, LDI_SHADER_C);
//    m_cudppRadixSortShader = LDIShader({m_shaderPath+"cCudppRadixSort.glsl"}, LDI_SHADER_C);
//    m_cudppFindRadixOffsetsShader = LDIShader({m_shaderPath+"cFindRadixOffsets.glsl"}, LDI_SHADER_C);
//    m_cudppReorderDataShader = LDIShader({m_shaderPath+"cReorderData.glsl"}, LDI_SHADER_C);
//    m_samScanShader = LDIShader({m_shaderPath+"cSamScan.glsl"}, LDI_SHADER_C);
//    m_testAOSShader = LDIShader({m_shaderPath+"vRasterizationOpt.glsl", m_shaderPath+"gRasterizationOpt.glsl",
//                                                         m_shaderPath+"fRasterization_AOS.glsl"}, LDI_SHADER_VGF);
//    m_testSOAShader = LDIShader({m_shaderPath+"vRasterizationOpt.glsl", m_shaderPath+"gRasterizationOpt.glsl",
//                                                         m_shaderPath+"fRasterization_SOA.glsl"}, LDI_SHADER_VGF);
//    //SVO Shaders
//    m_svoFlagShader = LDIShader({m_shaderPath+"cSVOFlag.glsl"}, LDI_SHADER_C);
//    glFinish();
}

LDIModel::~LDIModel()
{
    glDeleteBuffers(1, &m_ubo);
    glDeleteTextures(1, &m_renderColor);
    glDeleteTextures(1, &m_renderDepth);
    glDeleteFramebuffers(1, &m_fbo);
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

    // update internal data
    glBindBuffer(GL_UNIFORM_BUFFER, m_ubo);
    glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(glm::mat4), glm::value_ptr(projViewModelMat));
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
}

// Warning, change le viewport d'OpenGL
int LDIModel::getNbPixelFrags()
{
    glDisable(GL_DEPTH_TEST);
    glViewport(0,0,m_screenWidth,m_screenHeight);

    glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glUseProgram(m_fboPass.getProgramID());

    GLuint nbPixelsQuery;
    int nbPixels = 0;

    glGenQueries(1, &nbPixelsQuery);
    glBeginQuery(GL_SAMPLES_PASSED, nbPixelsQuery);
    draw();
    glEndQuery(GL_SAMPLES_PASSED);
    glGetQueryObjectiv(nbPixelsQuery, GL_QUERY_RESULT, &nbPixels);
    glDeleteQueries(1, &nbPixelsQuery);

    glEnable(GL_DEPTH_TEST);
    // recall glViewport ??

    return nbPixels;
}

std::vector<pixel_frag> LDIModel::getPixelFrags()
{
    glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);

    std::cout << "here" << std::endl;
    int nbPixels = getNbPixelFrags();

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
    return std::vector<pixel_frag>(0);
}

void LDIModel::draw()
{
    for(unsigned int i = 0; i < m_meshes.size(); ++i) {
        m_meshes[i]->draw();
    }
}
