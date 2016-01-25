#pragma once

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

    void            render(const glm::mat4& mvp
                          , GLuint vb
                          , size_t lineCount) const;


    static Ptr      instance();

private:
    LineShader(Shader::Ptr shader_
        , GLuint vertexPosition_
        , GLuint vertexColor_
        , GLuint projViewModel_);

    Shader::Ptr shader_;

    GLuint      vertexPosition_;
    GLuint      vertexColor_;

    GLuint      projViewModel_;
};

struct LineQueueView {
    typedef std::shared_ptr<LineQueueView> Ptr;
    
    struct Vertex {
        glm::vec3   position;
        glm::vec4   color;
    };

    static Ptr      create(size_t maxLineCount);

    void            addLine(const glm::vec3& v0, const glm::vec3& v1, const glm::vec4& color);
    void            flush();

private:
    LineQueueView(GLuint vb, size_t maxLineCount) : vb_(vb), maxLineCount_(maxLineCount) { verts_.resize(maxLineCount * 2); }


    std::vector<Vertex>     verts_;
    GLuint          vb_;
    size_t          maxLineCount_;
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
