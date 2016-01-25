#include "ObjLoader.hpp"

#include <iostream>
#include <stdio.h>

using namespace std;
using namespace glm;

struct objVertex {
    int     vId;
    int     nId;
    int     tId;
};

struct objTri {
    objVertex   v[3];
};

//
// adapted from: https://github.com/opengl-tutorials/ogl/blob/master/common/objloader.cpp
//
TriMesh::Ptr
loadFrom(const std::string& path) {
    cout << "Loading OBJ file " << path << "..." << endl;

    std::vector<unsigned int> vertexIndices, uvIndices, normalIndices;
    std::vector<glm::vec3> temp_vertices;
    std::vector<glm::vec2> temp_uvs;
    std::vector<glm::vec3> temp_normals;


    FILE * file = fopen(path.c_str(), "r");
    if (file == NULL) {
        cout << "Impossible to open the file ! Are you in the right path ? See Tutorial 1 for details" << endl;
        getchar();
        return nullptr;
    }

    while (1) {
        char lineHeader[1024];
        // read the first word of the line
        int res = fscanf(file, "%s", lineHeader);
        if (res == EOF)
            break; // EOF = End Of File. Quit the loop.

                   // else : parse lineHeader

        if (strcmp(lineHeader, "v") == 0) {
            glm::vec3 vertex;
            fscanf(file, "%f %f %f\n", &vertex.x, &vertex.y, &vertex.z);
            temp_vertices.push_back(vertex);
        }
        else if (strcmp(lineHeader, "vt") == 0) {
            glm::vec2 uv;
            fscanf(file, "%f %f\n", &uv.x, &uv.y);
            uv.y = -uv.y; // Invert V coordinate since we will only use DDS texture, which are inverted. Remove if you want to use TGA or BMP loaders.
            temp_uvs.push_back(uv);
        }
        else if (strcmp(lineHeader, "vn") == 0) {
            glm::vec3 normal;
            fscanf(file, "%f %f %f\n", &normal.x, &normal.y, &normal.z);
            temp_normals.push_back(normal);
        }
        else if (strcmp(lineHeader, "f") == 0) {
            std::string vertex1, vertex2, vertex3;
            unsigned int vertexIndex[3], uvIndex[3] = { 0, 0, 0 }, normalIndex[3];
            //int matches = fscanf(file, "%d/%d/%d %d/%d/%d %d/%d/%d\n", &vertexIndex[0], &uvIndex[0], &normalIndex[0], &vertexIndex[1], &uvIndex[1], &normalIndex[1], &vertexIndex[2], &uvIndex[2], &normalIndex[2]);
            int matches = fscanf(file, "%d//%d %d//%d %d//%d\n", &vertexIndex[0], &normalIndex[0], &vertexIndex[1], &normalIndex[1], &vertexIndex[2], &normalIndex[2]);
            if (matches != 6) {
                cout << "File can't be read by our simple parser :-( Try exporting with other options" << endl;
                return false;
            }
            vertexIndices.push_back(vertexIndex[0]);
            vertexIndices.push_back(vertexIndex[1]);
            vertexIndices.push_back(vertexIndex[2]);
            uvIndices.push_back(uvIndex[0]);
            uvIndices.push_back(uvIndex[1]);
            uvIndices.push_back(uvIndex[2]);
            normalIndices.push_back(normalIndex[0]);
            normalIndices.push_back(normalIndex[1]);
            normalIndices.push_back(normalIndex[2]);
        }
        else {
            // Probably a comment, eat up the rest of the line
            char stupidBuffer[1000];
            fgets(stupidBuffer, 1000, file);
        }

    }

    std::vector<TriMesh::Tri> tris;

    // For each vertex of each triangle
    for (unsigned int i = 0; i < vertexIndices.size() / 3; i++) {
        TriMesh::Tri tri;

        for (unsigned int v = 0; v < 3; ++v) {
            // Get the indices of its attributes
            unsigned int vertexIndex = vertexIndices[i * 3 + v];
            unsigned int uvIndex = uvIndices[i * 3 + v];
            unsigned int normalIndex = normalIndices[i * 3 + v];

            // Get the attributes thanks to the index
            tri.v[v].position = temp_vertices[vertexIndex - 1];
            //glm::vec2 uv = temp_uvs[uvIndex - 1];
            tri.v[v].normal = temp_normals[normalIndex - 1];
            tri.v[v].color = vec4(.5f, .5f, .5f, .5f);
        }
        // Put the attributes in buffers
        tris.push_back(tri);
    }



    return TriMesh::Ptr(new TriMesh(tris));
}