#include "../include/LDImodel.hpp"
#include <glm/gtx/component_wise.hpp>

template<typename T1, typename T2, typename T3>
void LDImodel<T1, T2, T3>::getIntervals(unsigned int render_dir)
{
    //glGenBuffers(1, &ssbo_box);
    //std::clock_t start = std::clock();
    m_getIntervalsShader.use();
    //ABOs
    initABO(&abo_nb_fragments, 0);
    //SSBOs
    //initSSBO<GLfloat>(&ssbo_lengths, m_nbPixels/2, 13);
    initSSBO<interval>(&ssbo_intervals, m_nbPixels/2, 14);
    bindSSBO(ssbo_optFrags, LDI_BINDING_OPT_FRAGS);
    bindSSBO(ssbo_prefixSums, LDI_BINDING_PREFIX_SUMS);
    //UNIFORMS
    glUniform1ui(glGetUniformLocation(m_getIntervalsShader.program, "NB_PIXELS"), m_nbPixels);
    bindUniformsGetPos(m_getIntervalsShader.program, render_dir);
    glDispatchCompute(m_nbPixels/1024 + 1, 1, 1);
    glFinish();
    //std::clock_t end = std::clock();
    //std::cout<<"temps getIntervals: "<<1000.0*double(end-start)/CLOCKS_PER_SEC<<std::endl;

    //unsigned int tmpAbo = getABO(abo_nb_fragments);
    //std::cout<<"getIntervalsFrags: abo vaut: "<<tmpAbo<<std::endl;
    //std::vector<GLfloat> tmpLengths = fetchSSBO<GLfloat>(ssbo_lengths, tmpAbo);
    //for(int i=0; i<tmpLengths.size(); i++)
    //    std::cout<<tmpLengths[i]<<std::endl;
}

template<typename T1, typename T2, typename T3>
void LDImodel<T1, T2, T3>::getVoxelFrags(unsigned int render_dir)
{
    std::clock_t start = std::clock();
    m_getVoxelFragsShader.use();
    //ABOs
    bindABO(abo_nb_fragments, 0);
    initABO(&abo_nb_voxel_frags, 1);
    //SSBOs
    //bindSSBO(ssbo_lengths, 13);
    initSSBO<GLfloat>(&ssbo_voxel_frags, 4*m_nbPixels, 13);
    bindSSBO(ssbo_intervals, 14);
    //UNIFORMS
    glUniform1ui((glGetUniformLocation(m_getVoxelFragsShader.program, "NB_PIXELS")), m_nbPixels);
    glUniform1ui((glGetUniformLocation(m_getVoxelFragsShader.program, "RENDER_DIR")), render_dir);
    glUniform1f((glGetUniformLocation(m_getVoxelFragsShader.program, "AREA")), m_pixelSurface);
    bindUniformsGetPos(m_getVoxelFragsShader.program, render_dir);
    unsigned int val_abo = getABO_named(abo_nb_fragments);
    //get the work per interval
    glDispatchCompute(val_abo/1024 + 1, 1, 1);
    //glBindBuffer(GL_DISPATCH_INDIRECT_BUFFER, ssbo_indirect);
    //glDispatchComputeIndirect(0);
    glFinish();

    std::clock_t end = std::clock();
    std::cout<<"temps getVoxelFrags: "<<1000.0*double(end-start)/CLOCKS_PER_SEC<<std::endl;

    unsigned int tmpAbo = getABO(abo_nb_fragments);
    std::cout<<"getVoxelFrags: abo vaut: "<<tmpAbo<<std::endl;
    unsigned int tmpVoxelsAbo = getABO(abo_nb_voxel_frags);
    std::cout<<"getVoxelFrags: voxelAbo vaut: "<<tmpVoxelsAbo<<std::endl;
    //std::vector<GLfloat> tmpLengths = fetchSSBO<GLfloat>(ssbo_lengths, tmpAbo);
    //for(int i=0; i<tmpLengths.size(); i++)
    //    std::cout<<tmpLengths[i]<<std::endl;
    //std::vector<GLuint> tmpScan = fetchSSBO<GLuint>(ssbo_scan_output, val_abo);
    //for(int i=0; i<100; i++)
    //    std::cout<<tmpScan[i]<<std::endl;
}

template<typename T1, typename T2, typename T3>
void LDImodel<T1, T2, T3>::updateOptMultiGradients_2(GLuint nb_divide)
{
    m_gotMultiGradients = true;
    initializeGradientsData();

    initSSBO<GLuint>(&ssbo_indirect, 4, 0);

    m_volume = 0;
    m_nb_divide = nb_divide;
    m_volumes.resize(m_nb_divide*m_nb_divide*m_nb_divide, 0.0f);
    m_multiGradients = std::vector<glm::vec3>(m_volumes.size()*m_totalNbVertex, glm::vec3(0));
    m_multiGradients_arrow = std::vector<glm::vec3>(m_volumes.size()*m_totalNbVertex*2, glm::vec3(0));
    //resize and std::fill !!

    std::clock_t begin = std::clock();
    //init Gradients
    m_zeroInitializationFloatShader.use();
    int total_nb_sommets = m_volumes.size()*(m_totalNbVertex+1)*3;
    initSSBO<GLfloat>(&ssbo_grad, total_nb_sommets, LDI_BINDING_ZERO_FLOAT);
    glUniform1ui((glGetUniformLocation(m_zeroInitializationFloatShader.program, "NB_PIXELS")), total_nb_sommets);
    glDispatchCompute(total_nb_sommets/256 + 1, 1, 1);

    double total_time = 0.0;
    std::clock_t end = std::clock();
    //std::cout<<"temps de préparation: "<<1000.0*double(end-begin)/CLOCKS_PER_SEC<<std::endl;
    total_time += 1000.0*double(end-begin);


    begin = std::clock();
    getNbPixels(m_xViewBox);
    glFinish();
    hashPixels();
    glFinish();
    buildPrefixSums();
    glFinish();
    rasterizationOpt();
    glFinish();
    bubbleSort();
    glFinish();
    getIntervals(0);
    //updateIndirectBuffer(abo_nb_fragments);
    getVoxelFrags(0);
    //buildSVO();
    glFinish();
    freeOptMultiGradientsBuffers();

    //begin = std::clock();
    //getNbPixels(m_yViewBox);
    //hashPixels();
    //buildPrefixSums();
    //rasterizationOpt();
    //bubbleSort();
    //glFinish();
    //computeOptMultiGradients(1, nb_divide);
    //freeOptBuffers();
    //end = std::clock();
    ////std::cout<<"temps total en y: "<<1000.0*double(end-begin)/CLOCKS_PER_SEC<<std::endl;
    //total_time += 1000.0*double(end-begin);

    //begin = std::clock();
    //getNbPixels(m_zViewBox);
    //hashPixels();
    //buildPrefixSums();
    //rasterizationOpt();
    //bubbleSort();
    //glFinish();
    ////std::clock_t begin_fin = std::clock();
    ////std::clock_t end_fin = std::clock();
    //computeOptMultiGradients(2, nb_divide);
    ////std::cout<<"temps de finish: "<<1000.0*double(end_fin-begin_fin)/CLOCKS_PER_SEC<<std::endl;
    //freeOptBuffers();
    //glFinish();
    //end = std::clock();
    ////std::cout<<"temps total en z: "<<1000.0*double(end-begin)/CLOCKS_PER_SEC<<std::endl;
    //total_time += 1000.0*double(end-begin);
    ////std::cout<<"total_time: "<<total_time/CLOCKS_PER_SEC<<std::endl;

    //begin = std::clock();
    //memset(m_multiGradients.data(), 0, m_multiGradients.size()*sizeof(float));
    //getOptMultiGradientsData();
    ////filterMultiGradients();
    ////getOptSparseMultiGradientsData();
    //end = std::clock();
    ////std::cout<<"temps pour récupérer les gradients: "<<1000.0*double(end-begin)/CLOCKS_PER_SEC<<std::endl;
    //total_time += 1000.0*double(end-begin);
    ////std::cout<<"total_time: "<<total_time/CLOCKS_PER_SEC<<std::endl;
    //initializeMultiGradientsArrow();
    //initializeBoxes();
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    //std::cout<<"m_volume: "<<m_volume<<std::endl;
}

template<typename T1, typename T2, typename T3>
void LDImodel<T1, T2, T3>::rasterizationOptAOS()
{
    int nb_frags = m_nbPixels;

    m_testAOSShader.use();
    initABO(&abo_nb_fragments, 0);
    //initialize shader context
    initSSBO<opt_frag>(&ssbo_optFrags, nb_frags, 13);
    bindOrthogonalOptView(m_testAOSShader.program, m_optProjMat, m_optViewMat,
                          m_optModelMat);
    glUniform1ui((glGetUniformLocation(m_testAOSShader.program, "H")), m_nbPixels);
    glUniform1i((glGetUniformLocation(m_testAOSShader.program, "SCREEN_WIDTH")), m_screenHeight);
    glUniform1ui((glGetUniformLocation(m_testAOSShader.program, "MAX_AGE")), m_max_age);
    //draw call
    int idObjLoc = glGetUniformLocation(m_testAOSShader.program, "ID_OBJ");
    for(int i=0; i<m_meshes.size(); i++)
    {
        glUniform1ui(idObjLoc, i);
        m_meshes[i]->draw();
    }
    //std::cout<<"rasterizationOptAOS: abo vaut: "<<getABO(abo_nb_fragments)<<std::endl;
    //std::cout<<"rasterizationOptAOS: m_nbPixels vaut: "<<m_nbPixels<<std::endl;
}

template<typename T1, typename T2, typename T3>
void LDImodel<T1, T2, T3>::rasterizationOptSOA()
{
    int nb_frags = m_nbPixels;

    m_testSOAShader.use();
    initABO(&abo_nb_fragments, 0);
    //initialize shader context
    GLuint ssbo_keys, ssbo_indices, ssbo_depths, ssbo_barys;
    initSSBO<GLfloat>(&ssbo_keys, nb_frags, 13);
    initSSBO<GLuint>(&ssbo_indices, nb_frags*4, 14);
    initSSBO<GLfloat>(&ssbo_depths, nb_frags, 15);
    initSSBO<GLfloat>(&ssbo_barys, nb_frags*2, 16);
    bindOrthogonalOptView(m_testSOAShader.program, m_optProjMat, m_optViewMat,
                          m_optModelMat);
    glUniform1ui((glGetUniformLocation(m_testSOAShader.program, "H")), m_nbPixels);
    glUniform1i((glGetUniformLocation(m_testSOAShader.program, "SCREEN_WIDTH")), m_screenHeight);
    glUniform1ui((glGetUniformLocation(m_testSOAShader.program, "MAX_AGE")), m_max_age);
    //draw call
    int idObjLoc = glGetUniformLocation(m_testSOAShader.program, "ID_OBJ");
    for(int i=0; i<m_meshes.size(); i++)
    {
        glUniform1ui(idObjLoc, i);
        m_meshes[i]->draw();
    }
    //std::cout<<"rasterizationOptAOS: abo vaut: "<<getABO(abo_nb_fragments)<<std::endl;
    //std::cout<<"rasterizationOptAOS: m_nbPixels vaut: "<<m_nbPixels<<std::endl;
}

template<typename T1, typename T2, typename T3>
void LDImodel<T1, T2, T3>::updateIndirectBuffer(GLuint abo)
{
    std::clock_t start = std::clock();
    m_indirectShader.use();
    bindABO(abo, 0);
    bindSSBO(ssbo_indirect, 0);
    glDispatchCompute(1, 1, 1);
    glFinish();
    //std::vector<GLuint> tmpIndirect = fetchSSBO<GLuint>(ssbo_indirect, 4);
    //for(int i=0; i<tmpIndirect.size(); i++)
    //    std::cout<<tmpIndirect[i]<<std::endl;
    //uint val = getABO(abo);
    //glFinish();
    std::clock_t end = std::clock();
    std::cout<<"temps updateIndirectBuffer: "<<1000.0*double(end-start)/CLOCKS_PER_SEC<<std::endl;
}

//template<typename T1, typename T2, typename T3>
//void LDImodel<T1, T2, T3>::buildSVO()
//{
//    GLint buffer_page_size = 0;
//    glGetIntegerv(GL_SPARSE_BUFFER_PAGE_SIZE_ARB, &buffer_page_size); //2^16 on my machine
//    GLintptr buffer_size = glm::ceilMultiple<GLint>(sizeof(GLuint), buffer_page_size);
//
//    glCreateBuffers(1, &ssbo_octree);
//    glNamedBufferStorage(ssbo_octree, 2*buffer_size, NULL, GL_SPARSE_STORAGE_BIT_ARB);
//    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, ssbo_octree);
//    GLint size = 0;
//    glGetBufferParameteriv(GL_SHADER_STORAGE_BUFFER, GL_BUFFER_SIZE, &size);
//    std::cout<<"allocated size: "<<size<<std::endl;
//
//    glBufferPageCommitmentARB(GL_SHADER_STORAGE_BUFFER, 0, buffer_size, GL_TRUE);
//    unsigned int nb_voxel_frags = getABO(abo_nb_voxel_frags);
//    std::cout<<"buildSVO: nb_voxel_frags: "<<nb_voxel_frags<<std::endl;
//    //get the max dimension of the scene's bounding box
//    float voxelDim = std::max(glm::compMax(glm::abs(m_scene_min)), glm::compMax(glm::abs(m_scene_min)));
//    std::cout<<"buildSVO: voxelDim: "<<voxelDim<<std::endl;
//    //std::vector<GLuint> tmpSVO = fetchSSBO<GLuint>(ssbo_octree, 10);
//    //void * data;
//    //glGetNamedBufferSubData(ssbo_octree, 0, buffer_size, data);
//    //printBinary(tmpSVO[0]);
//
//    std::clock_t start = std::clock();
//    m_svoFlagShader.use();
//    bindSSBO(ssbo_voxel_frags, 0);
//    bindSSBO(ssbo_octree, 1);
//    glUniform1ui(glGetUniformLocation(m_svoFlagShader.program, "OCTREE_LEVEL"), 0);
//    glUniform1f(glGetUniformLocation(m_svoFlagShader.program, "VOXEL_DIM"), voxelDim);
//    glUniform1ui(glGetUniformLocation(m_svoFlagShader.program, "NB_VOXEL_FRAGS"), nb_voxel_frags);
//    glDispatchCompute(nb_voxel_frags/16+1, 1, 1);
//    glFinish();
//    std::clock_t end = std::clock();
//    std::cout<<"temps buildSVO: "<<1000.0*double(end-start)/CLOCKS_PER_SEC<<std::endl;
//
//    //std::vector<GLuint> tmpSVO_2 = fetchSSBO<GLuint>(ssbo_octree, 128);
//    //printBinary(tmpSVO_2[0]);
//    GLuint ssbo_read;
//    initSSBO<GLuint>(&ssbo_read, 10, 0);
//    glBindBuffer(GL_COPY_WRITE_BUFFER, ssbo_read);
//    glBindBuffer(GL_COPY_READ_BUFFER, ssbo_octree);
//    glCopyBufferSubData(GL_COPY_READ_BUFFER, GL_COPY_WRITE_BUFFER, 0, 0, 10*sizeof(GLuint));
//    std::vector<GLuint> tmp = fetchSSBO<GLuint>(ssbo_read, 10);
//    std::cout<<"tmp[0]: ";
//    printBinary(tmp[0]);
//}

template<typename T1, typename T2, typename T3>
void LDImodel<T1, T2, T3>::getOptFragsFromIntervals()
{
    int real_nb_intervals = getABO(abo_nb_fragments);
    glFinish();
    //and we fetch the fragments
    m_optFrags.clear();
    fetchSSBO<opt_frag>(ssbo_intervals, 2*real_nb_intervals, m_optFrags);
    //std::vector<interval> tmpFrags = fetchSSBO<interval>(ssbo_optFrags, real_nb_frags/2);
    //m_optFrags.clear();
    //m_optFrags.resize(real_nb_frags);
    //for(int i=0; i<tmpFrags.size(); i++)
    //{
    //    m_optFrags[2*i] = tmpFrags[i].z0;
    //    m_optFrags[2*i+1] = tmpFrags[i].z1;
    //}
    glFinish();
}
