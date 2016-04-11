#pragma once

#include <glm/glm/glm.hpp>
#include <glm/glm/gtc/matrix_transform.hpp>
#include <glm/glm/gtc/matrix_inverse.hpp>
#include <glm/glm/gtx/intersect.hpp>
#include <glm/glm/gtx/quaternion.hpp>

struct TrackBall {

    TrackBall() : last_(glm::vec2()), current_(glm::vec2()), isOn_(false), wasPressed_(false) {}

    glm::quat update(const glm::vec2& pos, bool pressed, float width, float height);

private:
    glm::quat getRotation(float width, float height) const;

    static glm::vec3 project(glm::vec2 pos, float width, float height);

    glm::vec2   last_;
    glm::vec2   current_;
    bool        isOn_;
    bool        wasPressed_;
    glm::quat   rotation_;
};
