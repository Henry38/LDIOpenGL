#include "../include/LDImodel.hpp"
#include "../include/Utils.hpp"
#include <ctime>

template<typename T1, typename T2, typename T3>
void LDImodel<T1, T2, T3>::getNbPixels(box viewBox)
{
    m_passShader.use();
    //we simple request an occlusion query of the scene
    GLuint occl_query;
    glGenQueries(1, &occl_query);
    //GLuint ssbo_max_thread_id;
    initOrthogonalOptView(viewBox, m_optProjMat, m_optViewMat, m_optModelMat);
    bindOrthogonalOptView(m_passShader.program, m_optProjMat, m_optViewMat,
                          m_optModelMat);
    glBeginQuery(GL_SAMPLES_PASSED, occl_query);
    //draw call
    for(int i=0; i<m_meshes.size(); i++)
        m_meshes[i]->draw();
    glEndQuery(GL_SAMPLES_PASSED);
    //get the number of samples passed
    m_nbPixels = 0;
    glGetQueryObjectiv(occl_query, GL_QUERY_RESULT, &m_nbPixels);
    //std::cout<<"getNbPixels: m_nbPixels: "<<m_nbPixels<<std::endl;
}

template<typename T1, typename T2, typename T3>
void LDImodel<T1, T2, T3>::hashPixels()
{
    //initialization of the hashtable
    m_zeroInitializationUint64Shader.use();
    initSSBO<uint64_t>(&ssbo_pixelHashTable, m_nbPixels, LDI_BINDING_ZERO_UINT64);
    glUniform1ui(glGetUniformLocation(m_zeroInitializationUint64Shader.program, "NB_PIXELS"), m_nbPixels);
    glDispatchCompute(m_nbPixels/256 + 1, 1, 1);

    m_pixelHashShader.use();
    bindSSBO(ssbo_pixelHashTable, LDI_BINDING_HASHTABLE);
    initSSBO_offset(&ssbo_offsets, m_offsets, LDI_BINDING_OFFSETS);
    bindOrthogonalOptView(m_pixelHashShader.program, m_optProjMat, m_optViewMat,
                          m_optModelMat);
    glUniform1ui(glGetUniformLocation(m_pixelHashShader.program, "H"), m_nbPixels);
    //glUniform1i(glGetUniformLocation(m_pixelHashShader.program, "SCREEN_WIDTH"), m_screenWidth);
    glUniform1i(glGetUniformLocation(m_pixelHashShader.program, "SCREEN_WIDTH"), m_screenHeight);
    glUniform1ui(glGetUniformLocation(m_pixelHashShader.program, "MAX_AGE"), m_max_age);
    //draw call
    for(int i=0; i<m_meshes.size(); i++)
        m_meshes[i]->draw();
    glFinish();
}

template<typename T1, typename T2, typename T3>
void LDImodel<T1, T2, T3>::buildPrefixSums()
{
    //the bug is somewhere in here
    m_prefixSumsShader.use();
    bindSSBO(ssbo_pixelHashTable, 0);
    int nb_blocks = std::ceil(float(m_nbPixels)/1024.0f);
    initSSBO<GLuint>(&ssbo_prefixSums, m_nbPixels+1, 1);

    GLuint ssbo_gcarry, ssbo_gwait;
    initSSBO<GLuint>(&ssbo_gcarry, nb_blocks, 2);
    initSSBO<GLuint>(&ssbo_gwait, nb_blocks, 3);
    bindSSBO(ssbo_gcarry, 2);
    bindSSBO(ssbo_gwait, 3);
    glUniform1ui(glGetUniformLocation(m_prefixSumsShader.program, "NB_ELEMENTS"), m_nbPixels);

    glDispatchCompute(nb_blocks, 1, 1);
    glFinish();

    glDeleteBuffers(1, &ssbo_gcarry);
    glDeleteBuffers(1, &ssbo_gwait);
}

template<typename T1, typename T2, typename T3>
void LDImodel<T1, T2, T3>::rasterizationOpt()
{
    //it's too slow to get the actual number of fragments from the GPU,
    //so we will allocate a buffer slightly too large
    int nb_frags = m_nbPixels*2;
    m_zeroInitializationUintShader.use();
    initSSBO<GLuint>(&ssbo_atomicCounters, m_nbPixels, LDI_BINDING_ZERO_UINT);
    glUniform1ui(glGetUniformLocation(m_zeroInitializationUintShader.program, "NB_PIXELS"), m_nbPixels);
    glDispatchCompute(m_nbPixels/256 + 1, 1, 1);
    glFinish();

    m_rasterizationOptShader.use();
    //initialize shader context
    bindSSBO(ssbo_pixelHashTable, LDI_BINDING_HASHTABLE);
    bindSSBO(ssbo_offsets, LDI_BINDING_OFFSETS);
    bindSSBO(ssbo_prefixSums, LDI_BINDING_PREFIX_SUMS);
    bindSSBO(ssbo_atomicCounters, LDI_BINDING_ATOMIC_COUNTERS);
    initSSBO<opt_frag>(&ssbo_optFrags, nb_frags, LDI_BINDING_OPT_FRAGS);
    bindOrthogonalOptView(m_rasterizationOptShader.program, m_optProjMat, m_optViewMat,
                          m_optModelMat);
    glUniform1ui(glGetUniformLocation(m_rasterizationOptShader.program, "H"), m_nbPixels);
    glUniform1i(glGetUniformLocation(m_rasterizationOptShader.program, "SCREEN_WIDTH"), m_screenHeight);
    glUniform1ui(glGetUniformLocation(m_rasterizationOptShader.program, "MAX_AGE"), m_max_age);
    //draw call
    int idObjLoc = glGetUniformLocation(m_rasterizationOptShader.program, "ID_OBJ");
    for(int i=0; i<m_meshes.size(); i++)
    {
        glUniform1ui(idObjLoc, i);
        m_meshes[i]->draw();
    }
    glFinish();
}

template<typename T1, typename T2, typename T3>
void LDImodel<T1, T2, T3>::bubbleSort()
{
    m_bubbleSortShader.use();
    bindSSBO(ssbo_optFrags, LDI_BINDING_OPT_FRAGS);
    bindSSBO(ssbo_prefixSums, LDI_BINDING_PREFIX_SUMS);
    glUniform1ui(glGetUniformLocation(m_bubbleSortShader.program, "NB_PIXELS"), m_nbPixels);
    glDispatchCompute(m_nbPixels/256 + 1, 1, 1);
    glFinish();
}

template<typename T1, typename T2, typename T3>
void LDImodel<T1, T2, T3>::computeOptGradients(unsigned int render_dir)
{
    m_gradientsOptShader.use();
    //initABO(&abo_nb_fragments, 0);
    bindSSBO(ssbo_optFrags, LDI_BINDING_OPT_FRAGS);
    bindSSBO(ssbo_prefixSums, LDI_BINDING_PREFIX_SUMS);
    bindSSBO(ssbo_grad, LDI_BINDING_GRADIENTS);
    glUniform1ui(glGetUniformLocation(m_gradientsOptShader.program, "NB_PIXELS"), m_nbPixels);
    glUniform1ui(glGetUniformLocation(m_gradientsOptShader.program, "RENDER_DIR"), render_dir);
    glUniform1ui(glGetUniformLocation(m_gradientsOptShader.program, "NB_VERTICES"), m_totalNbVertex+1);
    glUniform1f(glGetUniformLocation(m_gradientsOptShader.program, "AREA"), m_pixelSurface);
    if(render_dir == 0){
        glUniform1f(glGetUniformLocation(m_gradientsOptShader.program, "BOX_DEPTH"), m_xViewBox.depth);
    } else if(render_dir == 1) {
        glUniform1f(glGetUniformLocation(m_gradientsOptShader.program, "BOX_DEPTH"), m_yViewBox.depth);
    } else if(render_dir == 2){
        glUniform1f(glGetUniformLocation(m_gradientsOptShader.program, "BOX_DEPTH"), m_zViewBox.depth);
    }
    glDispatchCompute(m_nbPixels/1024 + 1, 1, 1);
    //std::cout<<"computeGradients: abo vaut: "<<getABO(abo_nb_fragments)<<std::endl;
}

template<typename T1, typename T2, typename T3>
void LDImodel<T1, T2, T3>::getOptFrags()
{
    //we fetch the number of fragments, which is in the PrefixSums-buffer
    std::vector<GLuint> vPrefixSums;
    fetchSSBO<GLuint>(ssbo_prefixSums, m_nbPixels+1, vPrefixSums);
    int real_nb_frags = vPrefixSums[m_nbPixels];
    glFinish();
    m_optFrags.clear();
    fetchSSBO<opt_frag>(ssbo_optFrags, real_nb_frags, m_optFrags);
    glFinish();
}

template<typename T1, typename T2, typename T3>
void LDImodel<T1, T2, T3>::freeOptGradientsBuffers()
{
    glDeleteBuffers(1, &ssbo_pixelHashTable);
    glDeleteBuffers(1, &ssbo_prefixSums);
    glDeleteBuffers(1, &ssbo_optFrags);
    glDeleteBuffers(1, &ssbo_atomicCounters);
    glDeleteBuffers(1, &ssbo_offsets);
}

template<typename T1, typename T2, typename T3>
void LDImodel<T1, T2, T3>::freeOptMultiGradientsBuffers()
{
    freeOptGradientsBuffers();
    glDeleteBuffers(1, &ssbo_box);
    glDeleteBuffers(1, &ssbo_intervals);
}

template<typename T1, typename T2, typename T3>
void LDImodel<T1, T2, T3>::updateOptGradients()
{
    bindFBOWA(m_fbo, m_screenWidth, m_screenHeight);
    m_volume = 0;
    T1 zero_T1;
    zero_T1[0] = 0;
    zero_T1[1] = 0;
    zero_T1[2] = 0;
    std::fill(m_gradients.begin(), m_gradients.end(), zero_T1);

    //init Gradients
    m_zeroInitializationFloatShader.use();
    int total_nb_sommets = (m_totalNbVertex+1)*3;
    initSSBO<GLfloat>(&ssbo_grad, total_nb_sommets, LDI_BINDING_ZERO_FLOAT);
    glUniform1ui(glGetUniformLocation(m_zeroInitializationFloatShader.program, "NB_PIXELS"), total_nb_sommets);
    glDispatchCompute(total_nb_sommets/256 + 1, 1, 1);

    updateViewport(m_xViewBox);
    getNbPixels(m_xViewBox);
    hashPixels();
    buildPrefixSums();
    rasterizationOpt();
    bubbleSort();
    computeOptGradients(0);
    freeOptGradientsBuffers();
    resetViewport();

    updateViewport(m_yViewBox);
    getNbPixels(m_yViewBox);
    hashPixels();
    buildPrefixSums();
    rasterizationOpt();
    bubbleSort();
    computeOptGradients(1);
    freeOptGradientsBuffers();
    resetViewport();

    updateViewport(m_zViewBox);
    getNbPixels(m_zViewBox);
    hashPixels();
    buildPrefixSums();
    rasterizationOpt();
    bubbleSort();
    computeOptGradients(2);
    freeOptGradientsBuffers();
    resetViewport();
    resetViewport();

    getOptGradientsData();
    //filterGradients();
    //getOptSparseGradientsData();
    glDeleteBuffers(1, &ssbo_grad);
    glDeleteBuffers(1, &ssbo_sparseGrad);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

template<typename T1, typename T2, typename T3>
void LDImodel<T1, T2, T3>::getOptMultiGradientsData()
{
    //on récupère ssbo_grad, alors les gradients et le volume
    size_t nbGradients = (m_totalNbVertex+1)*m_volumes.size();
    std::vector<float> tmpGradients;
    fetchSSBO<float>(ssbo_grad, 3*nbGradients, tmpGradients);
    int renderStep, volumeStep;
    for(int renderDir=0; renderDir<3; renderDir++)
    {
        renderStep = renderDir*nbGradients;
        for(int v=0; v<m_volumes.size(); v++)
        {
            volumeStep = v*(m_totalNbVertex+1);
            m_volume += tmpGradients[renderStep+volumeStep]/3;
            //m_volumes[v] += tmpGradients[renderStep+volumeStep]/3;
            if(renderStep==0)
               m_volumes[v] += tmpGradients[renderStep+volumeStep];
            for(unsigned int i=0; i<m_totalNbVertex; i++)
            {
                if(tmpGradients[renderStep+volumeStep+i+1] != 0)
                {
                    m_multiGradients[v*m_totalNbVertex+i][renderDir] = tmpGradients[renderStep+volumeStep+i+1];
                }
            }
        }
    }
}

template<typename T1, typename T2, typename T3>
void LDImodel<T1, T2, T3>::getOptGradientsData()
{
    //on récupère ssbo_grad, alors les gradients et le volume
    size_t nbGradients = m_totalNbVertex+1;
    std::vector<float> tmpGradients;
    fetchSSBO<float>(ssbo_grad, 3*nbGradients, tmpGradients);
        //for(int i=0; i<nbGradients; i++)
        //    std::cout<<"x: "<<tmpGradients[i]<<", y: "<<tmpGradients[i+nbGradients]<<", z: "<<tmpGradients[i+2*nbGradients]<<std::endl;
    for(int renderDir=0; renderDir<3; renderDir++)
    {
        m_volume += tmpGradients[nbGradients*renderDir];
        for(unsigned int i=0; i<m_totalNbVertex; i++)
        {
            if(tmpGradients[nbGradients*renderDir+i+1] != 0)
            {
                m_gradients[i][renderDir] = tmpGradients[nbGradients*renderDir+i+1];
            }
        }
    }
    m_volume /= 3;
}

template<typename T1, typename T2, typename T3>
void LDImodel<T1, T2, T3>::getOptSparseGradientsData()
{
    int nb_gradients = getABO(abo_nb_fragments);
    //std::cout<<"getOptSparseGradientsData: abo_nb_fragments: "<<abo_nb_fragments<<std::endl;
    std::vector<float> tmpGradients;
    fetchSSBO<float>(ssbo_sparseGrad, 4*nb_gradients, tmpGradients);
    for(int i=0; i<tmpGradients.size(); i +=4)
    {
        int index = (int)tmpGradients[i];
        if(index == 0)
        {
            m_volume = (tmpGradients[i+1]+tmpGradients[i+2]+tmpGradients[i+3])/3.0f;
        } else {
            m_gradients[index-1][0] = tmpGradients[i+1];
            m_gradients[index-1][1] = tmpGradients[i+2];
            m_gradients[index-1][2] = tmpGradients[i+3];
        }
    }
}

template<typename T1, typename T2, typename T3>
void LDImodel<T1, T2, T3>::getOptSparseMultiGradientsData()
{
    int nb_gradients = getABO(abo_nb_fragments);
    std::vector<float> tmpGradients;
    fetchSSBO<float>(ssbo_sparseGrad, 4*nb_gradients, tmpGradients);
    for(int i=0; i<tmpGradients.size(); i +=4)
    {
        int key = (int)tmpGradients[i];
        int nb_volume = key/(m_totalNbVertex+1);
        int index = key-(m_totalNbVertex+1)*nb_volume;
        if(index == 0)
        {
            m_volume += (tmpGradients[i+1]+tmpGradients[i+2]+tmpGradients[i+3])/3.0f;
            m_volumes[nb_volume] = (tmpGradients[i+1]+tmpGradients[i+2]+tmpGradients[i+3])/3.0f;
        } else {
            m_multiGradients[m_totalNbVertex*nb_volume+index-1][0] = tmpGradients[i+1];
            m_multiGradients[m_totalNbVertex*nb_volume+index-1][1] = tmpGradients[i+2];
            m_multiGradients[m_totalNbVertex*nb_volume+index-1][2] = tmpGradients[i+3];
        }
    }
}


template<typename T1, typename T2, typename T3>
void LDImodel<T1, T2, T3>::filterGradients()
{
    m_sparseGradientsShader.use();
    bindSSBO(ssbo_grad, LDI_BINDING_GRADIENTS);
    initSSBO<GLfloat>(&ssbo_sparseGrad, (m_totalNbVertex+1)*4, LDI_BINDING_SPARSE_GRADIENTS);
    initABO(&abo_nb_fragments, 0);
    glUniform1ui(glGetUniformLocation(m_sparseGradientsShader.program, "NB_VERTICES"), m_totalNbVertex);

    glDispatchCompute((m_totalNbVertex+1)/256+1, 1, 1);
    glFinish();
}

template<typename T1, typename T2, typename T3>
void LDImodel<T1, T2, T3>::filterMultiGradients()
{
    m_sparseMultiGradientsShader.use();
    bindSSBO(ssbo_grad, LDI_BINDING_GRADIENTS);
    initSSBO<GLfloat>(&ssbo_sparseGrad, m_volumes.size()*(m_totalNbVertex+1)*4, LDI_BINDING_SPARSE_GRADIENTS);
    initABO(&abo_nb_fragments, 0);
    glUniform1ui(glGetUniformLocation(m_sparseMultiGradientsShader.program, "NB_VERTICES"), m_totalNbVertex);
    glUniform1ui(glGetUniformLocation(m_sparseMultiGradientsShader.program, "NB_VOLUMES"), m_volumes.size());

    glDispatchCompute((m_volumes.size()*(m_totalNbVertex+1))/256+1, 1, 1);
}


template<typename T1, typename T2, typename T3>
void LDImodel<T1, T2, T3>::computeBoxOpt(unsigned int render_dir)
{
    m_boxComputeOptShader.use();
    //SSBO_FRAGMENTS
    bindSSBO(ssbo_optFrags, LDI_BINDING_OPT_FRAGS);
    bindSSBO(ssbo_prefixSums, LDI_BINDING_PREFIX_SUMS);
    //SSBO_BOX
    initSSBO<GLint>(&ssbo_box, 6, LDI_BINDING_BBOX);
    //UNIFORMS
    glUniform1ui(glGetUniformLocation(m_boxComputeOptShader.program, "RENDER_DIR"), render_dir);
    glUniform1ui(glGetUniformLocation(m_boxComputeOptShader.program, "NB_PIXELS"), m_nbPixels);
    bindUniformsGetPos(m_boxComputeOptShader.program, render_dir);

    glDispatchCompute(m_nbPixels/256+1, 1, 1);
    glFinish();
}

template<typename T1, typename T2, typename T3>
void LDImodel<T1, T2, T3>::bindUniformsGetPos(GLuint shaderProgram, int render_dir)
{
    glUniform1i(glGetUniformLocation(shaderProgram, "SCREEN_WIDTH"), m_screenWidth);
    glUniform1ui(glGetUniformLocation(shaderProgram, "SCREEN_HEIGHT"),m_screenHeight);
    if(render_dir == 0)
    {
        glUniform3f(glGetUniformLocation(shaderProgram, "a"), m_xViewBox.a[0],
                m_xViewBox.a[1], m_xViewBox.a[2]);
        glUniform3f(glGetUniformLocation(shaderProgram, "b"), m_xViewBox.b[0],
                m_xViewBox.b[1], m_xViewBox.b[2]);
        glUniform3f(glGetUniformLocation(shaderProgram, "c"), m_xViewBox.c[0],
                m_xViewBox.c[1], m_xViewBox.c[2]);
        glUniform3f(glGetUniformLocation(shaderProgram, "d"), m_xViewBox.d[0],
                m_xViewBox.d[1], m_xViewBox.d[2]);
        glUniform1f(glGetUniformLocation(shaderProgram, "BOX_DEPTH"), m_xViewBox.depth);
    } else if(render_dir == 1) {
        glUniform3f(glGetUniformLocation(shaderProgram, "a"), m_yViewBox.a[0],
                m_yViewBox.a[1], m_yViewBox.a[2]);
        glUniform3f(glGetUniformLocation(shaderProgram, "b"), m_yViewBox.b[0],
                m_yViewBox.b[1], m_yViewBox.b[2]);
        glUniform3f(glGetUniformLocation(shaderProgram, "c"), m_yViewBox.c[0],
                m_yViewBox.c[1], m_yViewBox.c[2]);
        glUniform3f(glGetUniformLocation(shaderProgram, "d"), m_yViewBox.d[0],
                m_yViewBox.d[1], m_yViewBox.d[2]);
        glUniform1f(glGetUniformLocation(shaderProgram, "BOX_DEPTH"), m_yViewBox.depth);
    } else if(render_dir == 2) {
        glUniform3f(glGetUniformLocation(shaderProgram, "a"), m_zViewBox.a[0],
                m_zViewBox.a[1], m_zViewBox.a[2]);
        glUniform3f(glGetUniformLocation(shaderProgram, "b"), m_zViewBox.b[0],
                m_zViewBox.b[1], m_zViewBox.b[2]);
        glUniform3f(glGetUniformLocation(shaderProgram, "c"), m_zViewBox.c[0],
                m_zViewBox.c[1], m_zViewBox.c[2]);
        glUniform3f(glGetUniformLocation(shaderProgram, "d"), m_zViewBox.d[0],
                m_zViewBox.d[1], m_zViewBox.d[2]);
        glUniform1f(glGetUniformLocation(shaderProgram, "BOX_DEPTH"), m_zViewBox.depth);
    }
}

template<typename T1, typename T2, typename T3>
bounding_box LDImodel<T1, T2, T3>::getVOIBoundingBox()
{
    //getting the bounding box along the x-direction
    updateViewport(m_xViewBox);
    bindFBOWA(m_fbo, m_screenWidth, m_screenHeight);
    getNbPixels(m_xViewBox);
    hashPixels();
    buildPrefixSums();
    rasterizationOpt();
    bubbleSort();
    computeBoxOpt(0);
    std::vector<GLint> tmpBox;
    fetchSSBO<GLint>(ssbo_box, 6, tmpBox);
    float magnitude = 100000.0f;
    glm::vec3 boxMin = glm::vec3(tmpBox[0]/magnitude, tmpBox[1]/magnitude, tmpBox[2]/magnitude);
    glm::vec3 boxMax = glm::vec3(tmpBox[3]/magnitude, tmpBox[4]/magnitude, tmpBox[5]/magnitude);
    bounding_box res;
    res.min = boxMin;
    res.max = boxMax;
    //if(boxMin.x > m_scene_max.x || boxMin.y > m_scene_max.y || boxMin.z > m_scene_max.z)
    //{
    //    res.min = glm::vec3(0);
    //    res.max = glm::vec3(0);
    //}
    freeOptMultiGradientsBuffers();
    debindFBOWA();
    resetViewport();
    //getting the bounding box along the y-direction
    updateViewport(m_yViewBox);
    bindFBOWA(m_fbo, m_screenWidth, m_screenHeight);
    getNbPixels(m_yViewBox);
    hashPixels();
    buildPrefixSums();
    rasterizationOpt();
    bubbleSort();
    computeBoxOpt(1);
    fetchSSBO<GLint>(ssbo_box, 6, tmpBox);
    boxMin = glm::vec3(tmpBox[0]/magnitude, tmpBox[1]/magnitude, tmpBox[2]/magnitude);
    boxMax = glm::vec3(tmpBox[3]/magnitude, tmpBox[4]/magnitude, tmpBox[5]/magnitude);
    res.min = glm::min(res.min, boxMin);
    res.max = glm::max(res.max, boxMax);
    freeOptMultiGradientsBuffers();
    debindFBOWA();
    resetViewport();
    //getting the bounding box along the z-direction
    updateViewport(m_zViewBox);
    bindFBOWA(m_fbo, m_screenWidth, m_screenHeight);
    getNbPixels(m_zViewBox);
    hashPixels();
    buildPrefixSums();
    rasterizationOpt();
    bubbleSort();
    computeBoxOpt(2);
    fetchSSBO<GLint>(ssbo_box, 6, tmpBox);
    boxMin = glm::vec3(tmpBox[0]/magnitude, tmpBox[1]/magnitude, tmpBox[2]/magnitude);
    boxMax = glm::vec3(tmpBox[3]/magnitude, tmpBox[4]/magnitude, tmpBox[5]/magnitude);
    res.min = glm::min(res.min, boxMin);
    res.max = glm::max(res.max, boxMax);
    freeOptMultiGradientsBuffers();
    debindFBOWA();
    resetViewport();
    if(res.min == glm::vec3(100))
    {
        res.min = glm::vec3(0);
        res.max = glm::vec3(0);
    }
    return res;
}

template<typename T1, typename T2, typename T3>
void LDImodel<T1, T2, T3>::updateOptMultiGradients(GLuint nb_divide)
{
    bindFBOWA(m_fbo, m_screenWidth, m_screenHeight);
    m_volume = 0;
    m_nb_divide = nb_divide;
    m_volumes.resize(m_nb_divide*m_nb_divide*m_nb_divide);
    std::fill(m_volumes.begin(), m_volumes.end(), 0.0f);
    m_multiGradients.resize(m_volumes.size()*m_totalNbVertex);
    T1 zero_T1;
    zero_T1[0] = 0;
    zero_T1[1] = 0;
    zero_T1[2] = 0;
    std::fill(m_multiGradients.begin(), m_multiGradients.end(), zero_T1);

    //init Gradients
    m_zeroInitializationFloatShader.use();
    int total_nb_sommets = m_volumes.size()*(m_totalNbVertex+1)*3;
    initSSBO<GLfloat>(&ssbo_grad, total_nb_sommets, LDI_BINDING_ZERO_FLOAT);
    glUniform1ui(glGetUniformLocation(m_zeroInitializationFloatShader.program, "NB_PIXELS"), total_nb_sommets);
    glDispatchCompute(total_nb_sommets/256 + 1, 1, 1);

    updateViewport(m_xViewBox);
    getNbPixels(m_xViewBox);
    hashPixels();
    buildPrefixSums();
    rasterizationOpt();
    bubbleSort();
    computeBoxOpt(0);
    computeOptMultiGradients(0, nb_divide);
    freeOptMultiGradientsBuffers();
    resetViewport();

    updateViewport(m_yViewBox);
    getNbPixels(m_yViewBox);
    hashPixels();
    buildPrefixSums();
    rasterizationOpt();
    bubbleSort();
    computeBoxOpt(1);
    computeOptMultiGradients(1, nb_divide);
    freeOptMultiGradientsBuffers();
    resetViewport();

    updateViewport(m_zViewBox);
    getNbPixels(m_zViewBox);
    hashPixels();
    buildPrefixSums();
    rasterizationOpt();
    bubbleSort();
    computeBoxOpt(2);
    computeOptMultiGradients(2, nb_divide);
    freeOptMultiGradientsBuffers();
    resetViewport();

    //memset(m_multiGradients.data(), 0, m_multiGradients.size()*sizeof(float));
    //std::fill(m_multiGradients.begin(), m_multiGradients.end(), zero_T1);
    getOptMultiGradientsData();
    //filterMultiGradients();
    //getOptSparseMultiGradientsData();
    glDeleteBuffers(1, &ssbo_grad);
    glDeleteBuffers(1, &ssbo_sparseGrad);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

template<typename T1, typename T2, typename T3>
void LDImodel<T1, T2, T3>::computeOptMultiGradients(unsigned int render_dir, GLuint nb_divide)
{
    m_multiGradientsOptShader.use();
    //initABO(&abo_nb_fragments, 0);
    //SSBOs
    bindSSBO(ssbo_optFrags, LDI_BINDING_OPT_FRAGS);
    bindSSBO(ssbo_prefixSums, LDI_BINDING_PREFIX_SUMS);
    bindSSBO(ssbo_grad, LDI_BINDING_GRADIENTS);
    bindSSBO(ssbo_box, LDI_BINDING_BBOX);
    //UNIFORMS
    glUniform1ui(glGetUniformLocation(m_multiGradientsOptShader.program, "NB_PIXELS"), m_nbPixels);
    glUniform1ui(glGetUniformLocation(m_multiGradientsOptShader.program, "RENDER_DIR"), render_dir);
    glUniform1ui(glGetUniformLocation(m_multiGradientsOptShader.program, "NB_VERTICES"), m_totalNbVertex);
    glUniform1f(glGetUniformLocation(m_multiGradientsOptShader.program, "AREA"), m_pixelSurface);
    glUniform1ui(glGetUniformLocation(m_multiGradientsOptShader.program, "NB_DIVIDE"), nb_divide);
    bindUniformsGetPos(m_multiGradientsOptShader.program, render_dir);
    glDispatchCompute(m_nbPixels/1024 + 1, 1, 1);
    glFinish();
    //std::cout<<"computeOptMultiGradients: abo vaut: "<<getABO(abo_nb_fragments)<<std::endl;
}

template<typename T1, typename T2, typename T3>
void LDImodel<T1, T2, T3>::getNbPixelFrags(glm::vec3 camCenter, glm::vec3 normal, glm::vec3 upDir, float height, float width,
                                   float depth)
{
    m_passShader.use();
    //initABO(&abo_nb_fragments, 0);
    //we simple request an occlusion query of the scene
    GLuint occl_query;
    glGenQueries(1, &occl_query);
    initOrthogonalOptFragsView(m_optProjMat, m_optViewMat, m_optModelMat, camCenter, normal,
                               upDir, height, width, depth);
    bindOrthogonalOptView(m_passShader.program, m_optProjMat, m_optViewMat,
                          m_optModelMat);
    glBeginQuery(GL_SAMPLES_PASSED, occl_query);
    //draw call
    for(int i=0; i<m_meshes.size(); i++)
        m_meshes[i]->draw();
    glFinish();
    glEndQuery(GL_SAMPLES_PASSED);
    //get the number of samples passed
    m_nbPixels = 0;
    glGetQueryObjectiv(occl_query, GL_QUERY_RESULT, &m_nbPixels);
    glDeleteQueries(1, &occl_query);
    //std::cout<<"getNbPixelFrags: abo vaut: "<<getABO(abo_nb_fragments)<<std::endl;
}
