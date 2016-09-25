#include "LDImodel.hpp"
//#include <cstring>
//#include <ctime>
//#include <iostream>

LDImodel::LDImodel(std::vector<LDIMesh*> vLDIMeshes, GLuint screenWidth, GLuint screenHeight) :
    m_meshes(vLDIMeshes),
    m_screenWidth(screenWidth),
    m_screenHeight(screenHeight),
//    m_Render(0), m_Display(0),
//    m_Direction(0), m_gotGradients(false), m_gotMultiGradients(false),
//    m_displayGradientsNb(0), m_nb_divide(5), m_tessInnerLvl(1), m_tessOuterLvl(1),
//    ssbo_out_mat(0), ssbo_radixCounters(0), ssbo_radixScannedCounters(0), ssbo_atomicCounters(0),
//    ssbo_box(0), ssbo_counter(0), ssbo_fragments(0), ssbo_grad(0), ssbo_hashTable(0),
//    ssbo_iData(0), ssbo_indirect(0), ssbo_intervals(0), ssbo_lengths(0), ssbo_octree(0), ssbo_oData(0),
//    ssbo_offsets(0), ssbo_optFrags(0), ssbo_pixelHashTable(0), ssbo_pixel_frag(0), ssbo_prefixSums(0),
//    ssbo_radixBlockOffsets(0), ssbo_reorderedData(0), ssbo_sparseGrad(0), ssbo_surface(0),
//    ssbo_voxel_frags(0), abo_nb_fragments(0), abo_nb_voxel_frags(0)
{
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

    initFBOWA(&m_fbo, m_screenWidth, m_screenHeight);

    LDIShader::addInclude(m_shaderPath + "include/structures.hglsl");
    LDIShader::addInclude(m_shaderPath + "include/buffers.hglsl");
    LDIShader::addInclude(m_shaderPath + "include/extensions.hglsl");
    LDIShader::addInclude(m_shaderPath + "include/uniforms.hglsl");
    LDIShader::addInclude(m_shaderPath + "include/auxiliary_functions.hglsl");
    LDIShader::addInclude(m_shaderPath + "include/tessellationEvaluation.hglsl");

    m_indirectShader = LDIShader({m_shaderPath+"cIndirect.glsl"}, LDI_SHADER_C);
    m_passShader = LDIShader({m_shaderPath+"vPass.glsl", m_shaderPath+"fPass.glsl"}, LDI_SHADER_VF);
    m_pixelHashShader = LDIShader({m_shaderPath+"vPass.glsl", m_shaderPath+"fHashPixels.glsl"}, LDI_SHADER_VF);
    m_zeroInitializationUint64Shader = LDIShader({m_shaderPath+"cZeroInitializationUint64.glsl"}, LDI_SHADER_C);
    m_zeroInitializationUintShader = LDIShader({m_shaderPath+"cZeroInitializationUint.glsl"}, LDI_SHADER_C);
    m_zeroInitializationFloatShader = LDIShader({m_shaderPath+"cZeroInitializationFloat.glsl"}, LDI_SHADER_C);
    m_prefixSumsShader = LDIShader({m_shaderPath+"cSamScanUint64.glsl"}, LDI_SHADER_C);
    m_blocSumsShader = LDIShader({m_shaderPath+"cBlocSum.glsl"}, LDI_SHADER_C);
    m_addBlocSumsShader = LDIShader({m_shaderPath+"cAddBlocSum.glsl"}, LDI_SHADER_C);
    m_rasterizationOptShader = LDIShader({m_shaderPath+"vRasterizationOpt.glsl", m_shaderPath+"gRasterizationOpt.glsl",
                                                         m_shaderPath+"fRasterizationOpt.glsl"}, LDI_SHADER_VGF);
    m_bubbleSortShader = LDIShader({m_shaderPath+"cBubbleSort.glsl"}, LDI_SHADER_C);
    m_gradientsOptShader = LDIShader({m_shaderPath+"cGradientsOpt.glsl"}, LDI_SHADER_C);
    //MULTI_GRADIENT Shaders
    m_sparseGradientsShader = LDIShader({m_shaderPath+"cSparseGradients.glsl"}, LDI_SHADER_C);
    m_boxComputeOptShader = LDIShader({m_shaderPath+"cBoxOpt.glsl"}, LDI_SHADER_C);
    m_multiGradientsOptShader = LDIShader({m_shaderPath+"cMultiGradientsOpt.glsl"}, LDI_SHADER_C);
    m_sparseMultiGradientsShader = LDIShader({m_shaderPath+"cSparseMultiGradients.glsl"}, LDI_SHADER_C);
    m_getIntervalsShader = LDIShader({m_shaderPath+"cGetIntervals.glsl"}, LDI_SHADER_C);
    m_getVoxelFragsShader = LDIShader({m_shaderPath+"cGetVoxelFrags.glsl"}, LDI_SHADER_C);
    m_getVoxelFragsDistribShader = LDIShader({m_shaderPath+"cGetVoxelFragsDistrib.glsl"}, LDI_SHADER_C);
    //RADIX_SORT Shaders
    m_transposeShader = LDIShader({m_shaderPath+"cTranspose.glsl"}, LDI_SHADER_C);
    m_cudppRadixSortShader = LDIShader({m_shaderPath+"cCudppRadixSort.glsl"}, LDI_SHADER_C);
    m_cudppFindRadixOffsetsShader = LDIShader({m_shaderPath+"cFindRadixOffsets.glsl"}, LDI_SHADER_C);
    m_cudppReorderDataShader = LDIShader({m_shaderPath+"cReorderData.glsl"}, LDI_SHADER_C);
    m_samScanShader = LDIShader({m_shaderPath+"cSamScan.glsl"}, LDI_SHADER_C);
    m_testAOSShader = LDIShader({m_shaderPath+"vRasterizationOpt.glsl", m_shaderPath+"gRasterizationOpt.glsl",
                                                         m_shaderPath+"fRasterization_AOS.glsl"}, LDI_SHADER_VGF);
    m_testSOAShader = LDIShader({m_shaderPath+"vRasterizationOpt.glsl", m_shaderPath+"gRasterizationOpt.glsl",
                                                         m_shaderPath+"fRasterization_SOA.glsl"}, LDI_SHADER_VGF);
    //SVO Shaders
    m_svoFlagShader = LDIShader({m_shaderPath+"cSVOFlag.glsl"}, LDI_SHADER_C);
    glFinish();
}


std::vector<pixel_frag> LDImodel::getPixelFrags_2(glm::vec3 camCenter, glm::vec3 normal, glm::vec3 upDir, float height, float width,
                                              float depth, float x_resolution, float y_resolution)
{
    //scaling of the viewport to get the desired resolution
    GLsizei height_nb = (int)(height/y_resolution);
    GLsizei width_nb = (int)(width/x_resolution);
    glGetIntegerv(GL_VIEWPORT, m_viewport);
    glViewport(0, 0, width_nb, height_nb);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    m_screenWidth = width_nb;
    m_screenHeight = height_nb;
    bindFBOWA(m_fbo, m_screenWidth, m_screenHeight);
    //rasterization
    getNbPixelFrags(camCenter, normal, upDir, height, width, depth);
    //std::cout<<"getPixelFrags_2: m_nbPixels: "<<m_nbPixels<<std::endl;
    if(m_nbPixels == 0)
    {
        //deleteFBOWA(&m_fbo);
        debindFBOWA();
        return m_pixelFrags;
    }
    unsigned int estimated_memory_usage = 16*4 + m_nbPixels*8 +
            m_nbPixels*4 + (m_nbPixels/2048+1)*4 + m_nbPixels*4 + m_nbPixels*64;
    //std::cout<<"estimated_memory_usage: "<<estimated_memory_usage<<std::endl;
    if(estimated_memory_usage/1000 > getCurrentAvailableMemory()*0.8)
        return std::vector<pixel_frag>(0);
    hashPixels();
    buildPrefixSums();
    rasterizationOpt();
    bubbleSort();
    //fetch the output
    getOptFrags();
    m_pixelFrags.clear();
    m_pixelFrags.resize(m_optFrags.size());
    //std::cout<<m_screenHeight<<", "<<m_screenWidth<<std::endl;
    for(int i=0; i<m_pixelFrags.size(); i++)
    {
        GLuint key = m_optFrags[i].info_2[0];
        GLuint m_i = key/m_screenHeight;
        GLuint m_j = key-m_i*m_screenHeight;
        m_pixelFrags[i].m_i = m_i;
        m_pixelFrags[i].m_j = m_j;
        m_pixelFrags[i].m_z = m_optFrags[i].info_2[1];
        m_pixelFrags[i].m_idObj = m_optFrags[i].info_1[0] & 0x3FFFFFFF;
        //std::cout<<m_i<<", "<<m_j<<", "<<m_pixelFrags[i].m_z<<std::endl;
        //std::cout<<m_optFrags[i].info_3[0]<<std::endl;
    }
    freeOptGradientsBuffers();
    //deleteFBOWA(&m_fbo);
    debindFBOWA();
    resetViewport();
    return m_pixelFrags;
}



std::vector<pixel_frag> LDImodel::getPixelFrags(box viewBox, float width, float height)
{
    //scaling of the viewport to get the desired resolution
    float width_box = glm::length(viewBox.b - viewBox.a);
    float height_box = glm::length(viewBox.d - viewBox.a);
    GLsizei height_nb = (int)(height_box/height);
    GLsizei width_nb = (int)(width_box/width);
    glViewport(0, 0, width_nb, height_nb);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    m_screenWidth = width_nb;
    m_screenHeight = height_nb;
    //initFBOWA(&m_fbo, m_screenWidth, m_screenHeight);
    bindFBOWA(m_fbo, m_screenWidth, m_screenHeight);
    //rasterization
    //getNbPixels(viewBox);
    getNbPixels(viewBox);
    hashPixels();
    buildPrefixSums();
    rasterizationOpt();
    bubbleSort();
    //fetch the output
    getOptFrags();
    m_pixelFrags.clear();
    m_pixelFrags.resize(m_optFrags.size());
    for(int i=0; i<m_pixelFrags.size(); i++)
    {
        GLuint key = m_optFrags[i].info_2[0];
        GLuint m_i = key/m_screenWidth;
        GLuint m_j = key-m_i*m_screenWidth;
        m_pixelFrags[i].m_i = m_i;
        m_pixelFrags[i].m_j = m_j;
        m_pixelFrags[i].m_z = m_optFrags[i].info_2[1];
        //std::cout<<m_i<<", "<<m_j<<", "<<m_pixelFrags[i].m_z<<std::endl;
    }
    freeOptGradientsBuffers();
    //deleteFBOWA(&m_fbo);
    debindFBOWA();
    return m_pixelFrags;
}

LDImodel::~LDImodel()
{
    glDeleteBuffers(1, &ssbo_out_mat);
    glDeleteBuffers(1, &ssbo_radixCounters);
    glDeleteBuffers(1, &ssbo_radixScannedCounters);
    glDeleteBuffers(1, &ssbo_fragments);
    glDeleteBuffers(1, &ssbo_hashTable);
    glDeleteBuffers(1, &ssbo_offsets);
    glDeleteBuffers(1, &ssbo_grad);
    glDeleteBuffers(1, &ssbo_indirect);
    glDeleteBuffers(1, &ssbo_pixel_frag);
    glDeleteBuffers(1, &ssbo_counter);
    glDeleteBuffers(1, &ssbo_grad);
    glDeleteBuffers(1, &ssbo_surface);
    glDeleteBuffers(1, &ssbo_box);
    glDeleteBuffers(1, &ssbo_pixelHashTable);
    glDeleteBuffers(1, &ssbo_prefixSums);
    glDeleteBuffers(1, &ssbo_optFrags);
    glDeleteBuffers(1, &ssbo_atomicCounters);
    glDeleteBuffers(1, &ssbo_sparseGrad);
    glDeleteBuffers(1, &ssbo_voxel_frags);
    glDeleteBuffers(1, &ssbo_intervals);
    glDeleteBuffers(1, &ssbo_lengths);
    glDeleteBuffers(1, &ssbo_iData);
    glDeleteBuffers(1, &ssbo_oData);
    glDeleteBuffers(1, &ssbo_radixBlockOffsets);
    glDeleteBuffers(1, &ssbo_reorderedData);
    glDeleteBuffers(1, &ssbo_octree);
    glDeleteBuffers(1, &abo_nb_fragments);
    glDeleteBuffers(1, &abo_nb_voxel_frags);
}
