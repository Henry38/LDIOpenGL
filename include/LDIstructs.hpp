#ifndef LDISTRUCTS_HPP
#define LDISTRUCTS_HPP

#include <GL/glew.h>
#include <GL/glext.h>
#include <GL/glcorearb.h>

#include <glm/glm.hpp>
#include <vector>

//to differenciate between the different jobs, we use an enum
enum JOB {VOXELIZATION, GRADIENT, MULTI_GRADIENT};

struct double_frite {
    GLfloat topFrite[3];
    GLfloat downFrite[3];
};

struct pixel_frag {
    GLfloat m_i;
    GLfloat m_j;
    GLfloat m_z;
    GLuint m_idObj;
};

struct voxel_frag {
   GLuint idx_top_sommets[4];
   GLuint down_sommets[4];
   GLfloat dz_top_coeffs[4];
   GLfloat down_coeffs[4]; //vec4 to get aligned memory structure
};

//for the orthographic projection
struct box {
    glm::vec3 a, b, c, d;
    float depth;
    box():a(glm::vec3()),b(glm::vec3()),c(glm::vec3()),d(glm::vec3()), depth(0)
    {}
};

//structure auxiliaire pour le passage ssbo->CPU
struct frag {
    GLuint m_object;
    GLfloat m_i;
    GLfloat m_j;
    GLfloat m_depth;
    GLfloat m_b1;
    GLfloat m_b2;
    GLfloat m_b3;
    GLuint m_in;
    GLfloat m_world_x;
    GLfloat m_world_y;
    GLfloat m_world_z;
    GLuint m_triangle_sommet1;
    GLuint m_triangle_sommet2;
    GLuint m_triangle_sommet3;
};

struct opt_frag {
    GLuint info_1[4]; //in+depth, sommet_1, sommet_2, sommet_3
    GLfloat info_2[4]; //i*width+j, b_1, b_2, b_3
};

struct cell {
    float m_volume;
    std::vector<glm::vec3> m_gradients;
};

struct interval {
    opt_frag z0;
    opt_frag z1;
};

struct bounding_box {
    glm::vec3 min, max;
};

#endif
