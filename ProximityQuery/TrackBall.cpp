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

#include "TrackBall.hpp"

#include <vector>
#include <limits>
#include <cmath>

using namespace std;
using namespace glm;

TrackBall
TrackBall::update(const TrackBall& prev, const glm::vec2& pos, bool pressed, float width, float height) const {
    auto tb = TrackBall::from(prev);

    // arcball rotation routine
    if (!prev.wasPressed_ && pressed) {
        tb.isOn_ = true;
        tb.current_ = tb.last_ = pos;
    } else if (!pressed) {
        tb.isOn_ = false;
    }

    if (tb.isOn_) {
        tb.current_ = pos;

        if (tb.last_ != tb.current_) {
            auto newQuat = getRotation(width, height);
            tb.rotation_ = newQuat * prev.rotation_;
            tb.last_ = current_;
        }
    }

    tb.wasPressed_ = pressed;   // update

    return tb;
}

glm::quat
TrackBall::getRotation(float width, float height) const {
    glm::vec3 va = project(last_, width, height);
    glm::vec3 vb = project(current_, width, height);
    float t = min(1.0f, glm::dot(va, vb));
    glm::vec3 axis = glm::cross(va, vb);
    return glm::normalize(glm::quat(t, axis.x, axis.y, axis.z));
}

glm::vec3
TrackBall::project(glm::vec2 pos, float width, float height) {
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