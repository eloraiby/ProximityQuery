#include "TrackBall.hpp"

#include <vector>
#include <limits>
#include <cmath>

using namespace std;
using namespace glm;

glm::quat TrackBall::update(const glm::vec2& pos, bool pressed, float width, float height) {
    // arcball rotation routine
    if (!wasPressed_ && pressed) {
        isOn_ = true;
        current_ = last_ = pos;
    }
    else if (!pressed) {
        isOn_ = false;
    }

    if (isOn_) {
        current_ = pos;

        if (last_ != current_) {
            auto newQuat = getRotation(width, height);
            rotation_ = newQuat * rotation_;
            last_ = current_;
        }
    }

    wasPressed_ = pressed;   // update

    return rotation_;
}

glm::quat TrackBall::getRotation(float width, float height) const {
    glm::vec3 va = project(last_, width, height);
    glm::vec3 vb = project(current_, width, height);
    float t = min(1.0f, glm::dot(va, vb));
    glm::vec3 axis = glm::cross(va, vb);
    return glm::normalize(glm::quat(t, axis.x, axis.y, axis.z));
}

glm::vec3 TrackBall::project(glm::vec2 pos, float width, float height) {
    float   r = 0.8f;
    float	dim = min(width, height);

    auto	pt = vec2(2.0f * (pos.x - width * 0.5f) / dim,
        2.0f * (pos.y - height * 0.5f) / dim);

    float d, t, z;

    d = sqrtf(pt.x * pt.x + pt.y * pt.y);
    if (d < r * (float)0.70710678118654752440)	/* Inside sphere */
        z = sqrt(r * r - d * d);
    else	/* On hyperbola */
    {
        t = r / (float)1.41421356237309504880;
        z = t * t / d;
    }
    return glm::normalize(vec3(pt.x, pt.y, z));
}