#include <iostream>
#include <vector>

#include <glm/glm/glm.hpp>
#include <glm/glm/gtx/intersect.hpp>

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "TriMesh.h"
#include "ObjLoader.hpp"

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

    auto mesh = loadFrom("monkey.obj");

    while (!glfwWindowShouldClose(window)) {

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}