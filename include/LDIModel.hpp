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

// Table de liaison OpenGL:
//      0 = m_ubo
//      1 = ac_countFrag
//      2 = ac_countPixel
//      3 = ssbo pixelHashTable
//      5 = ssbo_prefixSum
//      6 = ssbo_blockSum
//      7 = ssbo_pixelFrag
//      8 = ssbo_indexFrag

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
        pixel_frag() {
            this->m_i = 0;
            this->m_j = 0;
            this->m_z = 0.0f;
        }

        unsigned int m_i;
        unsigned int m_j;
        float m_z;
    };

    LDIModel(const std::vector<LDIMesh*> &vLDIMeshes, const orthoView &view, float rx, float ry);
    ~LDIModel();

    void setOrthogonalView(const orthoView &view);
    unsigned int getPixelPassed();
    std::vector<pixel_frag> getPixelFrag();

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
    LDIShader m_shaderCountPixelFrag;
    LDIShader m_shaderFillPixelHashTable;
    LDIShader m_shaderPrefixSum;
    LDIShader m_shaderBlockSum;
    LDIShader m_shaderAddBlockSum;
    LDIShader m_shaderPixelFrag;
    LDIShader m_shaderFillIndexFrag;
    LDIShader m_shaderSortPixelFrag;

private:
    LDIModel();

    void getNbPixelFrag(GLuint &ac_countFrag);
    void hashPixel(GLuint &ac_countPixel, GLuint &ssbo_pixelHashTable, unsigned int maxPixel);
    void prefixSum(GLuint &ssbo_prefixSum, GLuint &ssbo_blockSum, unsigned int maxPixel);
    void pixelFrag(GLuint &ssbo_pixelFrag, unsigned int nbPixelFrag);
    void indexFrag(GLuint &ssbo_indexFrag, unsigned int maxPixel, unsigned int nbPixel);
    void sortPixelFrag(unsigned int nbPixel);

    void draw();
};

typedef typename LDIModel::orthoView orthoView;
typedef typename LDIModel::pixel_frag pixel_frag;

#endif
