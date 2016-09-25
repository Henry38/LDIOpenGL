#include "../include/LDImodel.hpp"
#include "../include/Utils.hpp"
#include <glm/gtx/vector_angle.hpp>
#include <ctime>

template<typename T1, typename T2, typename T3>
void LDImodel<T1, T2, T3>::getNbPixelsDisplacement(box viewBox)
{
    m_passDisplacementShader.use();
    //we simply request an occlusion query of the scene
    GLuint occl_query;
    glGenQueries(1, &occl_query);
    initOrthogonalOptView(viewBox, m_optProjMat, m_optViewMat, m_optModelMat);
    bindOrthogonalOptView(m_passDisplacementShader.program, m_optProjMat, m_optViewMat,
            m_optModelMat);
    glUniform1f(glGetUniformLocation(m_passDisplacementShader.program, "tessInnerLvl"), m_tessInnerLvl);
    glUniform1f(glGetUniformLocation(m_passDisplacementShader.program, "tessOuterLvl"), m_tessOuterLvl);
    glBeginQuery(GL_SAMPLES_PASSED, occl_query);
    //draw call
    bindTexture(texture_displacement);
    for(int i=0; i<m_meshes.size(); i++)
        m_meshes[i]->drawPatches();
    glEndQuery(GL_SAMPLES_PASSED);
    //get the number of samples passed
    m_nbPixels = 0;
    glGetQueryObjectiv(occl_query, GL_QUERY_RESULT, &m_nbPixels);
    //std::cout<<"getNbPixelsDisplacement: m_nbPixels: "<<m_nbPixels<<std::endl;
}

template<typename T1, typename T2, typename T3>
void LDImodel<T1, T2, T3>::hashPixelsDisplacement()
{
    //initialization of the hashtable
    m_zeroInitializationUint64Shader.use();
    initSSBO<uint64_t>(&ssbo_pixelHashTable, m_nbPixels, LDI_BINDING_ZERO_UINT64);
    glUniform1ui(glGetUniformLocation(m_zeroInitializationUint64Shader.program, "NB_PIXELS"), m_nbPixels);
    glDispatchCompute(m_nbPixels/256 + 1, 1, 1);

    m_pixelHashDisplacementShader.use();
    bindSSBO(ssbo_pixelHashTable, LDI_BINDING_HASHTABLE);
    initSSBO_offset(&ssbo_offsets, m_offsets, LDI_BINDING_OFFSETS);
    bindOrthogonalOptView(m_pixelHashDisplacementShader.program, m_optProjMat, m_optViewMat,
            m_optModelMat);
    glUniform1ui(glGetUniformLocation(m_pixelHashDisplacementShader.program, "H"), m_nbPixels);
    glUniform1i(glGetUniformLocation(m_pixelHashDisplacementShader.program, "SCREEN_WIDTH"), m_screenHeight);
    glUniform1ui(glGetUniformLocation(m_pixelHashDisplacementShader.program, "MAX_AGE"), m_max_age);
    glUniform1f(glGetUniformLocation(m_pixelHashDisplacementShader.program, "tessInnerLvl"), m_tessInnerLvl);
    glUniform1f(glGetUniformLocation(m_pixelHashDisplacementShader.program, "tessOuterLvl"), m_tessOuterLvl);
    //draw call
    bindTexture(texture_displacement);
    for(int i=0; i<m_meshes.size(); i++)
        m_meshes[i]->drawPatches();
}

template<typename T1, typename T2, typename T3>
void LDImodel<T1, T2, T3>::rasterizationOptDisplacement()
{
    int nb_frags = m_nbPixels*3;
    m_zeroInitializationUintShader.use();
    initSSBO<GLuint>(&ssbo_atomicCounters, nb_frags, LDI_BINDING_ZERO_UINT);
    glUniform1ui(glGetUniformLocation(m_zeroInitializationUintShader.program, "NB_PIXELS"), nb_frags);
    glDispatchCompute(m_nbPixels*3/256 + 1, 1, 1);

    m_rasterizationOptDisplacementShader.use();
    //initialize shader context
    bindSSBO(ssbo_pixelHashTable, LDI_BINDING_HASHTABLE);
    bindSSBO(ssbo_offsets, LDI_BINDING_OFFSETS);
    bindSSBO(ssbo_prefixSums, LDI_BINDING_PREFIX_SUMS);
    bindSSBO(ssbo_atomicCounters, LDI_BINDING_ATOMIC_COUNTERS);
    initSSBO<opt_frag>(&ssbo_optFrags, nb_frags, LDI_BINDING_OPT_FRAGS);
    bindOrthogonalOptView(m_rasterizationOptDisplacementShader.program, m_optProjMat, m_optViewMat,
            m_optModelMat);
    glUniform1ui(glGetUniformLocation(m_rasterizationOptDisplacementShader.program, "H"), m_nbPixels);
    glUniform1i(glGetUniformLocation(m_rasterizationOptDisplacementShader.program, "SCREEN_WIDTH"), m_screenHeight);
    glUniform1ui(glGetUniformLocation(m_rasterizationOptDisplacementShader.program, "MAX_AGE"), m_max_age);
    glUniform1f(glGetUniformLocation(m_rasterizationOptDisplacementShader.program, "tessInnerLvl"), m_tessInnerLvl);
    glUniform1f(glGetUniformLocation(m_rasterizationOptDisplacementShader.program, "tessOuterLvl"), m_tessOuterLvl);
    //draw call
    bindTexture(texture_displacement);
    for(int i=0; i<m_meshes.size(); i++)
        m_meshes[i]->drawPatches();
}

template<typename T1, typename T2, typename T3>
void LDImodel<T1, T2, T3>::updateOptGradientsDisplacement()
{
    bindFBOWA(m_fbo, m_screenWidth, m_screenHeight);

    m_volume = 0;
    m_gotGradients = true;
    //init Gradients
    m_zeroInitializationFloatShader.use();
    int total_nb_sommets = (m_totalNbVertex+1)*3;
    initSSBO<GLfloat>(&ssbo_grad, total_nb_sommets, LDI_BINDING_ZERO_FLOAT);
    glUniform1ui(glGetUniformLocation(m_zeroInitializationFloatShader.program, "NB_PIXELS"), total_nb_sommets);
    glDispatchCompute(total_nb_sommets/256 + 1, 1, 1);

    updateViewport(m_xViewBox);
    getNbPixelsDisplacement(m_xViewBox);
    hashPixelsDisplacement();
    buildPrefixSums();
    rasterizationOptDisplacement();
    bubbleSort();
    glFinish();
    computeOptGradients(0);
    freeOptGradientsBuffers();
    resetViewport();

    updateViewport(m_yViewBox);
    getNbPixelsDisplacement(m_yViewBox);
    hashPixelsDisplacement();
    buildPrefixSums();
    rasterizationOptDisplacement();
    bubbleSort();
    glFinish();
    computeOptGradients(1);
    freeOptGradientsBuffers();
    resetViewport();

    updateViewport(m_zViewBox);
    getNbPixelsDisplacement(m_zViewBox);
    hashPixelsDisplacement();
    buildPrefixSums();
    rasterizationOptDisplacement();
    bubbleSort();
    glFinish();
    computeOptGradients(2);
    resetViewport();

    memset(m_gradients.data(), 0, m_gradients.size()*sizeof(float));
    filterGradients();
    getOptSparseGradientsData();
    glViewport(0, 0, m_viewport[2], m_viewport[3]);
    glDeleteBuffers(1, &ssbo_grad);
    glDeleteBuffers(1, &ssbo_sparseGrad);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    freeOptGradientsBuffers();
}

template<typename T1, typename T2, typename T3>
void LDImodel<T1, T2, T3>::setDisplacementShader(std::string filePath)
{
    LDIShader::addInclude(filePath, "displacement.glsl");
    m_passDisplacementShader = LDIShader({m_shaderPath+"displacement/vPassDisplacement.glsl",
                m_shaderPath+"displacement/tcPassDisplacement.glsl",
                m_shaderPath+"displacement/tePassDisplacement.glsl",
                m_shaderPath+"displacement/fPassDisplacement.glsl"}, LDI_SHADER_VTCTEF);
    m_pixelHashDisplacementShader = LDIShader({m_shaderPath+"displacement/vPassDisplacement.glsl",
                m_shaderPath+"displacement/tcPassDisplacement.glsl",
                m_shaderPath+"displacement/tePassDisplacement.glsl",
                m_shaderPath+"displacement/fHashPixelsDisplacement.glsl"}, LDI_SHADER_VTCTEF);
    m_rasterizationOptDisplacementShader = LDIShader({m_shaderPath+"displacement/vPassDisplacement.glsl",
                m_shaderPath+"displacement/tcPassDisplacement.glsl",
                m_shaderPath+"displacement/tePassDisplacement.glsl",
                m_shaderPath+"displacement/gRasterizationOptDisplacement.glsl",
                m_shaderPath+"displacement/fRasterizationOptDisplacement.glsl"}, LDI_SHADER_VTCTEGF);
    glFinish();
}

template<typename T1, typename T2, typename T3>
void LDImodel<T1, T2, T3>::setDisplacementTexture(std::string filePath)
{
    initTexture(&texture_displacement, filePath);
}

template<typename T1, typename T2, typename T3>
void LDImodel<T1, T2, T3>::setDisplacementTessellationLevel(int tessLvl)
{
    m_tessInnerLvl = tessLvl;
    m_tessOuterLvl = tessLvl;
}

template<typename T1, typename T2, typename T3>
std::vector<T1> LDImodel<T1, T2, T3>::getWeightedNormals(LDImeshPtr<T1, T2, T3> mesh)
{
    const std::vector<T2> & indices = mesh->getIndices();
    const std::vector<T1> & vertices = mesh->getVertices();
    std::vector<T1> normals(vertices.size());
    for(int i=0; i<normals.size(); i++)
    {
        normals[i][0] = 0;
        normals[i][1] = 0;
        normals[i][2] = 0;
    }

    for(int i=0; i<indices.size(); i++)
    {
        for(int k=0; k<3; k++)
        {
            glm::uvec3 triInd = indices[i];
            glm::vec3 v0(vertices[triInd[k]][0], vertices[triInd[k]][1], vertices[triInd[k]][2]);
            glm::vec3 v1(vertices[triInd[(k+1)%3]][0], vertices[triInd[(k+1)%3]][1], vertices[triInd[(k+1)%3]][2]);
            glm::vec3 v2(vertices[triInd[(k+2)%3]][0], vertices[triInd[(k+2)%3]][1], vertices[triInd[(k+2)%3]][2]);
            glm::vec3 v0v1 = glm::normalize(v2-v0);
            glm::vec3 v0v2 = glm::normalize(v1-v0);
            float angle = glm::angle(v0v2, v0v1);
            glm::vec3 weightedNormal = angle*glm::normalize(glm::cross(v0v2, v0v1));
            normals[triInd[k]][0] += weightedNormal.x;
            normals[triInd[k]][1] += weightedNormal.y;
            normals[triInd[k]][2] += weightedNormal.z;
        }
    }

    for(int i=0; i<normals.size(); i++)
    {
        glm::vec3 normalized = glm::normalize(glm::vec3(normals[i][0], normals[i][1], normals[i][2]));
        //glm::vec3 tmpV = glm::vec3(vertices[i][0], vertices[i][1], vertices[i][2]);
        //printVec3(normalized);
        //printVec3(tmpV);
        normals[i][0] = normalized.x;
        normals[i][1] = normalized.y;
        normals[i][2] = normalized.z;
    }
    return normals;
}

template<typename T1, typename T2, typename T3>
void LDImodel<T1, T2, T3>::correctMeshNormals()
{
    for(int i=0; i<m_meshes.size(); i++)
    {
        m_meshes[i]->deleteBuffers();
        std::vector<T1> normals = getWeightedNormals(m_meshes[i]);
        m_meshes[i]->setNormals(normals);
        m_meshes[i]->initializeBuffers();
    }
}
