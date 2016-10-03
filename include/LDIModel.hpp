#ifndef LDIMODEL_HPP
#define LDIMODEL_HPP

// Glm
#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

// Project
#include "LDIMesh.hpp"
#include "LDIShader.hpp"

enum LDI_ENUMS
{
    LDI_BINDING_HASHTABLE,
    LDI_BINDING_OFFSETS,
    LDI_BINDING_PREFIX_SUMS,
    LDI_BINDING_SUMS,
    LDI_BINDING_ATOMIC_COUNTERS,
    LDI_BINDING_OPT_FRAGS,
    LDI_BINDING_GRADIENTS,
    LDI_BINDING_SPARSE_GRADIENTS,
    LDI_BINDING_ZERO_FLOAT,
    LDI_BINDING_ZERO_UINT,
    LDI_BINDING_ZERO_UINT64,
    LDI_BINDING_BBOX,
    LDI_BINDING_FRITES
};

class LDIModel {
public:

    struct orthoView {
        glm::vec3 camCenter;
        glm::vec3 normalDir;
        glm::vec3 upDir;
        float width;
        float height;
        float depth;
    };

    struct pixel_frag {
        unsigned short m_i;
        unsigned short m_j;
        float m_z;
    };

    LDIModel();
    LDIModel(const std::vector<LDIMesh*> &vLDIMeshes, const orthoView &view, float rx, float ry);
    ~LDIModel();

    void setOrthogonalView(const orthoView &view);

    // get Pixels Frags
    unsigned int getNbPixelFrags();
    void hashPixels(unsigned int nbPixels);
    std::vector<pixel_frag> getPixelFrags();

    std::vector<LDIMesh*> m_meshes;
    orthoView m_view;
    float m_x_resolution;
    float m_y_resolution;
    unsigned int m_screenWidth;
    unsigned int m_screenHeight;

    GLuint m_ubo;
    GLuint m_fbo;
    GLuint m_renderColor;
    GLuint m_renderDepth;

    LDIShader m_shaderFrameBuffer;
    LDIShader m_shaderInitPixelHashTable;
    LDIShader m_shaderFillPixelHashTable;

//    std::vector<pixel_frag> getPixelFrags(box viewBox, float width, float height);
//    std::vector<pixel_frag> getPixelFrags_2(glm::vec3 camCenter, glm::vec3 normal, glm::vec3 upDir, float height, float width, float depth, float x_resolution, float y_resolution);

//    /////////////// DEV PART /////////////
//    void updateOptMultiGradients_2(GLuint nb_divide);
//    void updateOptGradients();
//    void updateOptMultiGradients(GLuint nb_divide);
//    void updateOptGradientsDisplacement();
//    void getOptFrags();
//    void getOptFragsFromIntervals();
//    void freeOptGradientsBuffers();
//    void freeOptMultiGradientsBuffers();
//    void getNbPixels(box viewBox);
//    void hashPixelsDisplacement();
//    void rasterizationOptDisplacement();

//    void setBoundingBox(glm::vec3 min, glm::vec3 max);
//    void setResolution(float resolution);
//    void updateViewport(box viewbox);
//    void resetViewport();
//    int m_nbPixels;
//    box m_xViewBox, m_yViewBox, m_zViewBox;


//    GLuint ssbo_out_mat, ssbo_radixCounters, ssbo_radixScannedCounters;
//    GLuint m_nbBlocks, m_nbElementsToSort;

private:
    void draw();
//    void initOrthogonalView();
//    void bindOrthogonalView();

//    /////////////// COMMON PART /////////////
//    int estimateNbFragments(box viewBox);
//    void updateIndirectBuffer(GLuint abo);

//    std::vector<LDIMesh*> m_meshes;
//    unsigned int m_screenHeight, m_screenWidth;
////    unsigned int m_nb_fragments;
////    int m_estimatedNbFragments, m_Render, m_Display, m_Direction;
////    GLuint xVAO, xVBO, yVAO, yVBO, zVAO, zVBO, m_fbo;
////    GLuint ssbo_fragments, ssbo_hashTable, ssbo_offsets;
////    GLuint ssbo_indirect;
////    GLint m_viewport[4];
////    std::string m_shaderPath;
//    LDIShader m_surfaceShader, m_indirectShader;
//    std::vector<uint32_t> m_offsets;
//    std::vector<uint64_t> m_vHash;

    /////////////// VOXELIZATION PART /////////////
//    void initializePixelFrags(glm::vec3 camCenter, glm::vec3 normal, glm::vec3 upDir, float height, float width,
//                              float depth, float x_resolution, float y_resolution,
//                              std::vector<pixel_frag> pixelFrags,
//                              GLuint &iVAO, GLuint &iVBO);

//    GLuint ssbo_pixel_frag, ssbo_counter;
//    LDIShader m_fritesComputeShader;
//    LDIShaderPtr m_fritesRasterShaderPtr;
//    std::vector<pixel_frag> xPixelFrags, yPixelFrags, zPixelFrags, m_pixelFrags;

//    /////////////// GRADIENT PART /////////////
//    void initializeFrites(std::vector<glm::vec3> fritesPoints, GLuint &iVAO, GLuint &iVBO);
//    void initializeGradientsData();
//    void initializeGradientsArrow();

//    bool m_gotGradients;
//    float m_volume, m_resolution;
//    GLuint gradVAO, gradVBO;
//    GLuint ssbo_grad, abo_nb_fragments, ssbo_surface;
//    LDIShader m_gradientComputeShader;
//    LDIShaderPtr m_gradientRasterShaderPtr;
//    std::vector<glm::vec3> m_gradients, m_gradients_arrow;
//    std::vector<glm::vec3> xFritesPoints, yFritesPoints, zFritesPoints;

//    /////////////// MULTI-GRADIENT PART /////////////
//    void initializeMultiGradientsArrow();
//    void buildSVO();

//    bool m_gotMultiGradients;//, m_gotBox;
//    unsigned int m_displayGradientsNb, m_totalNbVertex;
//    GLuint multiGradVAO, multiGradVBO, ssbo_box, m_nb_divide, abo_nb_voxel_frags;
//    LDIShader m_boxComputeShader, m_multiGradientsShader;
//    std::vector<float> m_volumes;
//    std::vector<glm::vec3> m_multiGradients, m_multiGradients_arrow;

//    /////////////// OPT-GRADIENT PART /////////////
//    void getOptGradientsData();
//    void getOptMultiGradientsData();
//    void getOptSparseGradientsData();
//    void getOptSparseMultiGradientsData();

//    void hashPixels();
//    void buildPrefixSums();
//    void rasterizationOpt();
//    void rasterizationOptAOS();
//    void rasterizationOptSOA();
//    void bubbleSort();
//    void computeOptGradients(unsigned int render_dir);
//    void filterGradients();
//    void filterMultiGradients();
//    void computeBoxOpt(unsigned int render_dir);
//    void computeOptMultiGradients(unsigned int render_dir, GLuint nb_divide);
//    void getIntervals(unsigned int render_dir);
//    void getVoxelFrags(unsigned int render_dir);

//    void bindUniformsGetPos(GLuint shaderProgram, int render_dir);

//    float m_pixelSurface;
//    GLuint ssbo_pixelHashTable, ssbo_prefixSums, ssbo_optFrags,
//        ssbo_atomicCounters, ssbo_sparseGrad, ssbo_voxel_frags, ssbo_intervals,
//        ssbo_lengths;
//    glm::mat4 m_optProjMat, m_optViewMat, m_optModelMat;
//    LDIShader m_passShader, m_pixelHashShader, m_zeroInitializationUint64Shader,
//        m_prefixSumsShader, m_blocSumsShader, m_addBlocSumsShader, m_rasterizationOptShader,
//    m_bubbleSortShader, m_gradientsOptShader, m_zeroInitializationUintShader,
//    m_zeroInitializationFloatShader, m_sparseGradientsShader, m_boxComputeOptShader,
//    m_multiGradientsOptShader, m_sparseMultiGradientsShader, m_getVoxelFragsShader,
//    m_getIntervalsShader, m_getVoxelFragsDistribShader;
//    std::vector<opt_frag> m_optFrags;

//    /////////////// DISPLACEMENT-MAP PART /////////////

//    std::vector<T1> getWeightedNormals(LDIMeshPtr<T1, T2, T3> mesh);
//    unsigned int m_tessInnerLvl, m_tessOuterLvl;
//    GLuint texture_displacement;
//    LDIShader m_passDisplacementShader, m_pixelHashDisplacementShader,
//     m_rasterizationOptDisplacementShader;

//    /////////////// RADIX SORT PART /////////////

//    GLuint ssbo_iData, ssbo_oData, ssbo_radixBlockOffsets, ssbo_reorderedData;
//    LDIShader m_transposeShader,
//        m_cudppRadixSortShader, m_cudppFindRadixOffsetsShader, m_samScanShader,
//    m_cudppReorderDataShader;

//    LDIShader m_testAOSShader, m_testSOAShader;

//    /////////////// SVO PART /////////////

//    glm::vec3 m_scene_min, m_scene_max;
//    GLuint ssbo_octree;
    //    LDIShader m_svoAllocShader, m_svoFlagShader, m_svoInitShader;
};

typedef typename LDIModel::orthoView orthoView;
typedef typename LDIModel::pixel_frag pixel_frag;

#endif
