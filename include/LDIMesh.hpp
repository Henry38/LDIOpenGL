#ifndef LDIMesh_HPP
#define LDIMesh_HPP

// Standard Library
#include <string>
#include <vector>

// OpenGL
#include <GL/glew.h>

class aiMesh;

class LDIMesh {

public:

    typedef std::vector<float> floatVector;
    typedef std::vector<unsigned int> uintVector;

    void draw();

    static LDIMesh* fromObj(const std::string &filename);
    static void destroy(LDIMesh *ldiMesh);

private:

    struct VAO {
        GLuint id;
        GLuint vbo_vertices;
        GLuint vbo_normals;
        GLuint vbo_colors;
        GLuint vbo_indices;

        VAO() { }

        void loadToGPU(floatVector &vertices, floatVector &normals, floatVector &colors, uintVector &indices, GLenum mode);
        void free();
    };

    struct MeshEntry {
        VAO vao;

        floatVector m_vertices;
        floatVector m_normals;
        floatVector m_colors;
        uintVector m_indices;

        MeshEntry(const aiMesh *mesh);
        ~MeshEntry();
    };

    LDIMesh(unsigned int numMeshEntries);
    ~LDIMesh();

    std::vector<MeshEntry*> m_meshEntries;
    //VAO m_vao;
};

#endif
