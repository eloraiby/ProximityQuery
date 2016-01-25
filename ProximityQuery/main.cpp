#include <iostream>
#include <vector>

#include <glm/glm/glm.hpp>
#include <glm/glm/gtc/matrix_transform.hpp>
#include <glm/glm/gtc/matrix_inverse.hpp>
#include <glm/glm/gtx/intersect.hpp>
#include <glm/glm/gtx/quaternion.hpp>

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "TriMesh.hpp"
#include "ObjLoader.hpp"
#include "Render.hpp"

using namespace glm;
using namespace std;

void errorCallback(int error, const char* descriptor) {
    cerr << "GLFW3 Error 0x" << std::hex << error << std::dec << " - " << descriptor << endl;
}

int main(int argc, char* argv[]) {
    if (!glfwInit()) {
        exit(EXIT_FAILURE);
    }

    glfwSetErrorCallback(errorCallback);
    
    GLFWwindow* window = glfwCreateWindow(640, 480, "Proximity Query", NULL, NULL);
    
    if (!window) {
        glfwTerminate();
        exit(EXIT_FAILURE);
    }

    glfwMakeContextCurrent(window);

    if (glewInit() != GLEW_OK) {
        cout << "unable to init glew" << endl;
        glfwTerminate();
        exit(EXIT_FAILURE);
    }

    if (!GLEW_VERSION_3_2) {
        cout << "opengl version not supported!" << endl;
        glfwTerminate();
        exit(EXIT_FAILURE);
    }

    auto mesh = loadFrom("monkey.obj");

    if (mesh == nullptr) {
        glfwTerminate();
        exit(EXIT_FAILURE);
    }
    else {
        cout << "Mesh Loaded!" << endl;
    }

    auto meshShader = TriMeshShader::instance();

    if (meshShader == nullptr) {
        cerr << "Error: unable to load mesh shader" << endl;
        glfwTerminate();
        exit(EXIT_FAILURE);
    }

    auto meshView = TriMeshView::from(mesh);
    if (meshView == nullptr) {
        cerr << "Error: unable to create meshView" << endl;
        glfwTerminate();
        exit(EXIT_FAILURE);
    }
    
    glEnable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);

    float   i = 0.0f;

    while (!glfwWindowShouldClose(window)) {
        int width, height;
        glfwGetFramebufferSize(window, &width, &height);

        glClearColor(0.5f, 0.5f, 0.5f, 0.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
        glViewport(0, 0, width, height);

        auto mv = lookAt(vec3(0.0f, 0.0f, -3.0f), vec3(0.0f, 0.0f, 0.0f), vec3(0.0f, 1.0f, 0.0f))
            * scale(mat4(1.0f), vec3(0.5f, 0.5f, 0.5f))
            * mat4(angleAxis(i, normalize(vec3(1.0f, 1.0f, 0.5f))));

        auto proj = perspective(glm::pi<float>() / 4.0f, width / (float)height, 1.0f, 1000.0f);

        meshView->render(proj, mv, vec3(0.0f, 0.0f, -20.0f));

        i += 0.01f;

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}