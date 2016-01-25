#include "Render.hpp"

#include <fstream>
#include <cassert>

using namespace std;
using namespace glm;

Shader::Ptr
Shader::fromStrings(const std::string& vs, const std::string& fs) {
    GLuint vsId = glCreateShader(GL_VERTEX_SHADER);
    GLuint fsId = glCreateShader(GL_FRAGMENT_SHADER);
    GLuint progId = glCreateProgram();
    const int BUFF_SIZE = 8192;
    int strLen = 0;
    char str[BUFF_SIZE] = { 0 };
    
    auto shader = Shader::Ptr(new Shader(vsId, fsId, progId));  // let the shader auto destruct on failure

    int vsLen = vs.length();
    const char* vsSource = vs.c_str();

    int fsLen = fs.length();
    const char* fsSource = fs.c_str();

    glShaderSource(vsId, 1, &vsSource, &vsLen);
    glShaderSource(fsId, 1, &fsSource, &fsLen);

    glCompileShader(vsId);   
    if (glGetError() != GL_NO_ERROR) {
        glGetShaderInfoLog(vsId, BUFF_SIZE, &strLen, str);
        cerr << "Error compiling vertex shader: " << endl << str << endl;
        return nullptr;
    }

    glCompileShader(fsId);
    if (glGetError() != GL_NO_ERROR) {
        glGetShaderInfoLog(fsId, BUFF_SIZE, &strLen, str);
        cerr << "Error compiling fragment shader: " << endl << str << endl;
        return nullptr;
    }

    glAttachShader(progId, vsId);
    glAttachShader(progId, fsId);

    glLinkProgram(progId);
    if (glGetError() != GL_NO_ERROR) {
        glGetProgramInfoLog(vsId, BUFF_SIZE, &strLen, str);
        cerr << "Error linking program: " << endl << str << endl;
        return nullptr;
    }

    return shader;
}

static std::string
loadFile(const std::string& path) {
    std::ifstream file(path);
    std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    return content;
}

Shader::Ptr
Shader::fromFiles(const std::string& vsPath, const std::string& fsPath) {
    auto vs = loadFile(vsPath);
    auto fs = loadFile(fsPath);
    auto res = Shader::fromStrings(vs, fs);

    if (res != nullptr) {
        std::cout << "shaders " << vsPath << " and " << fsPath << " compiled OK..." << endl;
    }

    return res;
}

////////////////////////////////////////////////////////////////////////////////

LineShader::LineShader(Shader::Ptr shader
    , GLuint vertexPosition
    , GLuint vertexColor)

    : shader_(shader)
    , vertexPosition_(vertexPosition)
    , vertexColor_(vertexColor)
{}

LineShader::Ptr
LineShader::instance() {
    static LineShader::Ptr  sInstance = nullptr;
    if (sInstance == nullptr) {
        auto shader = Shader::fromFiles("vsLine.glsl", "fsLine.glsl");

        if (shader == nullptr) {
            return nullptr;
        }

        const int MAX_BUFF = 8192;
        char buff[MAX_BUFF] = { 0 };
        int strLen = 0;
        int size = 0;
        GLenum type = 0;

        GLint numActiveAttribs = 0;
        GLint numActiveUniforms = 0;
        glGetProgramiv(shader->program(), GL_ACTIVE_ATTRIBUTES, &numActiveAttribs);
        glGetProgramiv(shader->program(), GL_ACTIVE_UNIFORMS, &numActiveUniforms);

        for (int i = 0; i < numActiveAttribs; ++i) {
            glGetActiveAttrib(shader->program(), i, MAX_BUFF, &strLen, &size, &type, buff);
            cout << "Vertex Attrib (" << i << "): " << buff << endl;
        }

        auto vertexPosition = glGetAttribLocation(shader->program(), "vertexPosition");
        assert(glGetError() == GL_NO_ERROR);

        auto vertexColor = glGetAttribLocation(shader->program(), "vertexColor");
        assert(glGetError() == GL_NO_ERROR);

        sInstance = Ptr(new LineShader(shader, vertexPosition, vertexColor));
    }

    return sInstance;
}

void
LineShader::render(GLuint vb, size_t lineCount) const {
    glUseProgram(shader_->program());
    glBindBuffer(GL_ARRAY_BUFFER, vb);

    glVertexAttribPointer(vertexPosition_, 4, GL_FLOAT, GL_FALSE, sizeof(LineQueueView::Vertex), reinterpret_cast<void*>(offsetof(LineQueueView::Vertex, position)));
    glVertexAttribPointer(vertexColor_, 4, GL_FLOAT, GL_FALSE, sizeof(LineQueueView::Vertex), reinterpret_cast<void*>(offsetof(LineQueueView::Vertex, color)));

    glEnableVertexAttribArray(vertexPosition_);
    glEnableVertexAttribArray(vertexColor_);

    glDrawArrays(GL_LINES, 0, lineCount * 2);

    glDisableVertexAttribArray(vertexPosition_);
    glDisableVertexAttribArray(vertexColor_);
}

////////////////////////////////////////////////////////////////////////////////

LineQueueView::Ptr
LineQueueView::create(size_t maxLineCount) {
    GLuint vb;
    glGenBuffers(1, &vb);

    glBindBuffer(GL_ARRAY_BUFFER, vb);
    glBufferData(GL_ARRAY_BUFFER, maxLineCount * 2 * sizeof(LineQueueView::Vertex), nullptr, GL_DYNAMIC_DRAW);
    if (glGetError() != GL_NO_ERROR) {
        cerr << "Error: couldn't create LineQueueView" << endl;
        return nullptr;
    }

    return Ptr(new LineQueueView(vb, maxLineCount));
}

LineQueueView::~LineQueueView() {
    glDeleteBuffers(1, &vb_);
    delete[] verts_;
}

void
LineQueueView::addLine(const glm::mat4& mvp, const glm::vec3& v0, const glm::vec3& v1, const glm::vec4& color) {
    if (lineCount_ + 1 > maxLineCount_) {
        flush();
        lineCount_ = 0;
    }

    verts_[lineCount_ * 2] = Vertex(mvp * vec4(v0, 1.0f), color);
    verts_[lineCount_ * 2 + 1] = Vertex(mvp * vec4(v1, 1.0f), color);
    ++lineCount_;
}

void
LineQueueView::flush() {
    glBindBuffer(GL_ARRAY_BUFFER, vb_);
    glBufferSubData(GL_ARRAY_BUFFER, 0, lineCount_ * 2 * sizeof(Vertex), verts_);
    LineShader::instance()->render(vb_, lineCount_);
    lineCount_ = 0;
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

////////////////////////////////////////////////////////////////////////////////

TriMeshShader::TriMeshShader(Shader::Ptr shader
    , GLuint vertexPosition
    , GLuint vertexNormal
    , GLuint vertexColor
    , GLuint projViewModel
    , GLuint lightPosition
    , GLuint lightColor
    , GLuint ambientColor)

    : shader_(shader)
    , vertexPosition_(vertexPosition)
    , vertexNormal_(vertexNormal)
    , vertexColor_(vertexColor)
    , projViewModel_(projViewModel)
    , lightPosition_(lightPosition)
    , lightColor_(lightColor)
    , ambientColor_(ambientColor)
{}

TriMeshShader::Ptr
TriMeshShader::instance() {
    static TriMeshShader::Ptr  sInstance = nullptr;
    if (sInstance == nullptr) {
        auto shader = Shader::fromFiles("vsMesh.glsl", "fsMesh.glsl");
        
        if (shader == nullptr) {
            return nullptr;
        }
            
        const int MAX_BUFF = 8192;
        char buff[MAX_BUFF] = { 0 };
        int strLen = 0;
        int size = 0;
        GLenum type = 0;
        
        GLint numActiveAttribs = 0;
        GLint numActiveUniforms = 0;
        glGetProgramiv(shader->program(), GL_ACTIVE_ATTRIBUTES, &numActiveAttribs);
        glGetProgramiv(shader->program(), GL_ACTIVE_UNIFORMS, &numActiveUniforms);

        for (int i = 0; i < numActiveAttribs; ++i) {
            glGetActiveAttrib(shader->program(), i, MAX_BUFF, &strLen, &size, &type, buff);
            cout << "Vertex Attrib (" << i << "): " << buff << endl;
        }

        for (int i = 0; i < numActiveUniforms; ++i) {
            glGetActiveUniform(shader->program(), i, MAX_BUFF, &strLen, &size, &type, buff);
            cout << "Uniform (" << i << "): " << buff << endl;
        }

        auto vertexPosition = glGetAttribLocation(shader->program(), "vertexPosition");
        assert(glGetError() == GL_NO_ERROR);

        auto vertexNormal = glGetAttribLocation(shader->program(), "vertexNormal");
        assert(glGetError() == GL_NO_ERROR);

        auto vertexColor = glGetAttribLocation(shader->program(), "vertexColor");
        assert(glGetError() == GL_NO_ERROR);

        auto projViewModel = glGetUniformLocation(shader->program(), "projViewModel");
        assert(glGetError() == GL_NO_ERROR);
        
        auto lightPosition = glGetUniformLocation(shader->program(), "lightPosition");
        assert(glGetError() == GL_NO_ERROR);

        auto lightColor = glGetUniformLocation(shader->program(), "lightColor");
        assert(glGetError() == GL_NO_ERROR);

        auto ambientColor = glGetUniformLocation(shader->program(), "ambientColor");
        assert(glGetError() == GL_NO_ERROR);

        sInstance = Ptr(new TriMeshShader(shader, vertexPosition, vertexNormal, vertexColor, projViewModel, lightPosition, lightColor, ambientColor));
    }

    return sInstance;
}

void
TriMeshShader::render(const glm::mat4& proj
    , const glm::mat4& mv
    , const glm::vec3& lightPos
    , GLuint vb
    , size_t triCount) const
{
    glUseProgram(shader_->program());
    glBindBuffer(GL_ARRAY_BUFFER, vb);
    
    glVertexAttribPointer(vertexPosition_, 3, GL_FLOAT, GL_FALSE, sizeof(TriMesh::Vertex), reinterpret_cast<void*>(offsetof(TriMesh::Vertex, position)));
    glVertexAttribPointer(vertexNormal_  , 3, GL_FLOAT, GL_FALSE, sizeof(TriMesh::Vertex), reinterpret_cast<void*>(offsetof(TriMesh::Vertex, normal)));
    glVertexAttribPointer(vertexColor_   , 4, GL_FLOAT, GL_FALSE, sizeof(TriMesh::Vertex), reinterpret_cast<void*>(offsetof(TriMesh::Vertex, color)));

    glEnableVertexAttribArray(vertexPosition_);
    glEnableVertexAttribArray(vertexNormal_);
    glEnableVertexAttribArray(vertexColor_);

    auto invMv = glm::inverse(mv);
    auto invLightPos = invMv * vec4(lightPos, 1.0f);
    auto mvp = proj * mv;

    glUniformMatrix4fv(projViewModel_, 1, GL_FALSE, &(mvp[0].x));
    glUniform3f(lightPosition_, invLightPos.x, invLightPos.y, invLightPos.z);
    glUniform4f(lightColor_, 0.5f, 0.5f, 0.5f, 1.0f);
    glUniform4f(ambientColor_, 1.0f, 1.0f, 1.0f, 1.0f);

    glDrawArrays(GL_TRIANGLES, 0, triCount * 3);

    glDisableVertexAttribArray(vertexPosition_);
    glDisableVertexAttribArray(vertexNormal_);
    glDisableVertexAttribArray(vertexColor_);
}

////////////////////////////////////////////////////////////////////////////////
TriMeshView::~TriMeshView() {
    glDeleteBuffers(1, &vb_);
}

TriMeshView::Ptr
TriMeshView::from(TriMesh::Ptr mesh) {
    GLuint vb;
    glGenBuffers(1, &vb);

    glBindBuffer(GL_ARRAY_BUFFER, vb);
    glBufferData(GL_ARRAY_BUFFER, mesh->tris().size() * sizeof(TriMesh::Tri), mesh->tris().data(), GL_STATIC_DRAW);
    if (glGetError() != GL_NO_ERROR) {
        cerr << "Error: couldn't create TriMeshView" << endl;
        return nullptr;
    }

    return Ptr(new TriMeshView(mesh->tris().size(), vb));
}

void
TriMeshView::render(const glm::mat4& proj, const glm::mat4& mv, const glm::vec3& eye) const {
    TriMeshShader::instance()->render(proj, mv, eye, vb_, triCount_);
}