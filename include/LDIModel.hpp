#ifndef LDIMODEL_HPP
#define LDIMODEL_HPP

// Glm
#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

// Project
#include "LDIMesh.hpp"
#include "LDIShader.hpp"

class LDIModel {
public:

//    enum LDI_ENUMS
//    {
//        LDI_BINDING_ATOMIC_COUNTERS,
//        LDI_BINDING_PREFIX_SUMS,
//        LDI_BINDING_HASHTABLE,
//        LDI_BINDING_PIXELFRAG,
//    };

    struct orthoView
    {
        glm::vec3 camCenter;
        glm::vec3 normalDir;
        glm::vec3 upDir;
        float width;
        float height;
        float depth;
    };

    struct pixel_frag
    {
        unsigned int m_i;
        unsigned int m_j;
        float m_z;
    };

    LDIModel(const std::vector<LDIMesh*> &vLDIMeshes, const orthoView &view, float rx, float ry);
    ~LDIModel();

    void setOrthogonalView(const orthoView &view);
    std::vector<pixel_frag> getPixelFrags();

    std::vector<LDIMesh*> m_meshes;
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
    LDIShader m_shaderInitPrefixSum;
    LDIShader m_shaderPrefixSum;
    LDIShader m_shaderInitPixelFrag;
    LDIShader m_shaderPixelFrag;

private:
    LDIModel();

    // getPixelFrag
    unsigned int getNbPixelFrag();
    void hashPixel(GLuint &ssbo_pixelHashTable, unsigned int maxPixel);
    void prefixSum(GLuint &ssbo_prefixSum, unsigned int maxPixel);
    void pixelFrag(GLuint &ssbo_pixelFrag, unsigned int nbPixelFrag);

    void draw();
};

typedef typename LDIModel::orthoView orthoView;
typedef typename LDIModel::pixel_frag pixel_frag;

#endif
