#pragma once
//
// Triangular Mesh Proximity Query
// Copyright(C) 2016 Wael El Oraiby
// 
// This program is free software : you can redistribute it and / or modify
// it under the terms of the GNU Affero General Public License as
// published by the Free Software Foundation, either version 3 of the
// License, or (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.See the
// GNU Affero General Public License for more details.
// 
// You should have received a copy of the GNU Affero General Public License
// along with this program.If not, see <http://www.gnu.org/licenses/>.
//
#include <memory>
#include <iostream>
#include <string>

#include <GL/glew.h>

#include "TriMesh.hpp"

struct Shader {
    typedef std::shared_ptr<Shader> Ptr;

    ~Shader() {
        if (vs_) { glDeleteShader(vs_); }
        if (fs_) { glDeleteShader(fs_); }
        if (prog_) { glDeleteProgram(prog_); }
    }

    GLuint      program() const { return prog_; }

    static Ptr  fromStrings(const std::string& vs, const std::string& fs);
    static Ptr  fromFiles(const std::string& vsPath, const std::string& fsPath);

protected:
    Shader(GLuint vs, GLuint fs, GLuint prog) : vs_(vs), fs_(fs), prog_(prog) {}

    GLuint      vs_;
    GLuint      fs_;
    GLuint      prog_;
};

struct LineShader {
    typedef std::shared_ptr<LineShader> Ptr;

    void            render( GLuint vb
                          , size_t lineCount) const;


    static Ptr      instance();

private:
    LineShader(Shader::Ptr shader_
        , GLuint vertexPosition_
        , GLuint vertexColor_);

    Shader::Ptr shader_;

    GLuint      vertexPosition_;
    GLuint      vertexColor_;
};

struct LineQueueView {
    typedef std::shared_ptr<LineQueueView> Ptr;
    
    struct Vertex {
        glm::vec4   position;
        glm::vec4   color;

        Vertex() {}
        Vertex(const glm::vec4& p, const glm::vec4& c) : position(p), color(c) {}
    };

    ~LineQueueView();

    void            queueLine(const glm::mat4& mvp, const glm::vec3& v0, const glm::vec3& v1, const glm::vec4& color);

    void            queueCircleXY(const glm::mat4& mvp, float radius, float step, const glm::vec4& color);
    void            queueCircleYZ(const glm::mat4& mvp, float radius, float step, const glm::vec4& color);
    void            queueCircleXZ(const glm::mat4& mvp, float radius, float step, const glm::vec4& color);

    void            queueCirclesXYZ(const glm::mat4& mvp, float radius, float step, const glm::vec4& color);

    void            queueCube(const glm::mat4& mvp, const AABB& bbox, bool showInterior, const glm::vec4& color);

    void            flush();

    static Ptr      create(size_t maxLineCount);

private:
    LineQueueView(GLuint vb, size_t maxLineCount) : verts_(new Vertex[maxLineCount * 2]), vb_(vb), maxLineCount_(maxLineCount), lineCount_(0) {}

    Vertex*         verts_;
    GLuint          vb_;
    size_t          maxLineCount_;
    size_t          lineCount_;
};

////////////////////////////////////////////////////////////////////////////////

struct TriMeshShader {
    typedef std::shared_ptr<TriMeshShader> Ptr;

    void            render( const glm::mat4& proj
                          , const glm::mat4& mv
                          , const glm::vec3& lightPos
                          , GLuint vb
                          , size_t triCount) const;


    static Ptr      instance();

private:
    TriMeshShader( Shader::Ptr shader_
                 , GLuint vertexPosition_
                 , GLuint vertexNormal_
                 , GLuint vertexColor_
                 , GLuint projViewModel_
                 , GLuint lightPosition_
                 , GLuint lightColor_
                 , GLuint ambientColor_);

    Shader::Ptr shader_;

    GLuint      vertexPosition_;
    GLuint      vertexNormal_;
    GLuint      vertexColor_;
    
    GLuint      projViewModel_;
    GLuint      lightPosition_;
    GLuint      lightColor_;
    GLuint      ambientColor_;
};

struct TriMeshView {
    typedef std::shared_ptr<TriMeshView> Ptr;

    ~TriMeshView();

    void            render(const glm::mat4& proj, const glm::mat4& mv, const glm::vec3& eye) const;

    static Ptr      from(TriMesh::Ptr m);

private:

    TriMeshView(size_t triCount, GLuint vb) : triCount_(triCount), vb_(vb) {}
    
    size_t          triCount_;
    GLuint          vb_;
};

struct CollisionMeshView {
    typedef std::shared_ptr<CollisionMeshView> Ptr;

    void            render(const glm::mat4& proj, const glm::mat4& mv, const glm::vec3& eye) const;
    void            renderLeaves(LineQueueView::Ptr queue, const glm::mat4& proj, const glm::mat4& mv) const;

    static Ptr      from(CollisionMesh::Ptr m);

private:
    CollisionMeshView(size_t rootIdx, const std::vector<AABBNode>& leaves, const std::vector<TriMeshView::Ptr>& triMeshes) : rootIdx_(rootIdx), leaves_(leaves), triMeshes_(triMeshes) {}

    size_t          rootIdx_;
    std::vector<AABBNode>   leaves_;
    std::vector<TriMeshView::Ptr>   triMeshes_;
};
