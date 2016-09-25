#include "LDIMesh.hpp"

// Assimp
#include "assimp/Importer.hpp"
#include "assimp/postprocess.h"
#include "assimp/scene.h"

#include <iostream>

LDIMesh::LDIMesh(unsigned int numMeshEntries) :
    m_meshEntries(numMeshEntries)
{
}

LDIMesh::~LDIMesh()
{
    for (unsigned int i = 0; i < m_meshEntries.size(); ++i) {
        delete m_meshEntries[i];
    }
}

void LDIMesh::draw()
{
    for (unsigned int i = 0; i < m_meshEntries.size(); ++i) {
        MeshEntry *mesh = m_meshEntries[i];
        VAO *vao = &mesh->vao;
        glBindVertexArray(vao->id);
        glDrawElements(GL_TRIANGLES, 3*mesh->m_indices.size(), GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
    }
}

LDIMesh* LDIMesh::fromObj(const std::string &filename)
{
    Assimp::Importer importer;
    //const aiScene *scene = importer.ReadFile(filename, aiProcess_JoinIdenticalVertices | aiProcess_GenSmoothNormals);
    const aiScene *scene = importer.ReadFile(filename, aiProcess_GenNormals);
    if(!scene) {
        printf("Impossible d'importer le mesh: %s\n", importer.GetErrorString());
        return nullptr;
    }

    LDIMesh *ldiMesh = new LDIMesh(scene->mNumMeshes);

    for (unsigned int i = 0; i < scene->mNumMeshes; ++i) {
        aiMesh *mesh = scene->mMeshes[i];
        MeshEntry *meshEntry = new MeshEntry(mesh);
        ldiMesh->m_meshEntries[i] = meshEntry;
    }

    return ldiMesh;
}

void LDIMesh::destroy(LDIMesh *ldiMesh)
{
    delete ldiMesh;
}

LDIMesh::MeshEntry::MeshEntry(const aiMesh *mesh)
{
    if (mesh->HasPositions()) {
        m_vertices.resize(mesh->mNumVertices * 3);
        for (unsigned int i = 0; i < mesh->mNumVertices; ++i) {
            m_vertices[i * 3] = mesh->mVertices[i].x;
            m_vertices[i * 3 + 1] = mesh->mVertices[i].y;
            m_vertices[i * 3 + 2] = mesh->mVertices[i].z;
        }
    }

    if (mesh->HasNormals()) {
        m_normals.resize(mesh->mNumVertices * 3);
        for (unsigned int i = 0; i < mesh->mNumVertices; ++i) {
            m_normals[i * 3] = mesh->mNormals[i].x;
            m_normals[i * 3 + 1] = mesh->mNormals[i].y;
            m_normals[i * 3 + 2] = mesh->mNormals[i].z;
        }
    }

    if (mesh->HasPositions()) {
        m_colors.resize(mesh->mNumVertices * 3);
        for (unsigned int i = 0; i < mesh->mNumVertices; ++i) {
            m_colors[i * 3] = 0.8f;
            m_colors[i * 3 + 1] = 0.8f;
            m_colors[i * 3 + 2] = 0.8f;
        }
    }

    if (mesh->HasFaces()) {
        m_indices.resize(mesh->mNumFaces * 3);
        for (unsigned int i = 0; i < mesh->mNumFaces; ++i) {
            m_indices[3 * i] = mesh->mFaces[i].mIndices[0];
            m_indices[3 * i + 1] = mesh->mFaces[i].mIndices[1];
            m_indices[3 * i + 2] = mesh->mFaces[i].mIndices[2];
        }
    }

    vao.loadToGPU(m_vertices, m_normals, m_colors, m_indices, GL_STATIC_DRAW);
}

LDIMesh::MeshEntry::~MeshEntry()
{
    vao.free();
}

void LDIMesh::VAO::loadToGPU(floatVector &vertices, floatVector &normals, floatVector &colors, uintVector &indices, GLenum mode) {
    // Create some buffers inside the GPU memory
    glGenVertexArrays(1, &id);
    glGenBuffers(1, &vbo_vertices);
    glGenBuffers(1, &vbo_normals);
    glGenBuffers(1, &vbo_colors);
    glGenBuffers(1, &vbo_indices);

    // Activate VAO
    glBindVertexArray(id);

    // Store mesh positions into buffer inside the GPU memory
    glBindBuffer(GL_ARRAY_BUFFER, vbo_vertices);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), mode);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void *)0);
    glEnableVertexAttribArray(0);

    // Store mesh normals into buffer inside the GPU memory
    glBindBuffer(GL_ARRAY_BUFFER, vbo_normals);
    glBufferData(GL_ARRAY_BUFFER, normals.size() * sizeof(float), normals.data(), mode);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_TRUE, 0, (void *)0);
    glEnableVertexAttribArray(1);

    // Store mesh colors into buffer inside the GPU memory
    glBindBuffer(GL_ARRAY_BUFFER, vbo_colors);
    glBufferData(GL_ARRAY_BUFFER, colors.size() * sizeof(float), colors.data(), mode);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, (void *)0);
    glEnableVertexAttribArray(2);

    // Store mesh indices into buffer inside the GPU memory
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbo_indices);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), mode);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

void LDIMesh::VAO::free() {
    glDeleteBuffers(1, &vbo_vertices);
    glDeleteBuffers(1, &vbo_normals);
    glDeleteBuffers(1, &vbo_colors);
    glDeleteBuffers(1, &vbo_indices);
    glDeleteVertexArrays(1, &id);
}
