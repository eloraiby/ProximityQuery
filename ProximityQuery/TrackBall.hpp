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

#include <glm/glm/glm.hpp>
#include <glm/glm/gtc/matrix_transform.hpp>
#include <glm/glm/gtc/matrix_inverse.hpp>
#include <glm/glm/gtx/intersect.hpp>
#include <glm/glm/gtx/quaternion.hpp>

struct TrackBall {
    glm::quat           rotation() const { return rotation_; }

    static TrackBall    create() { return TrackBall(); }
    static TrackBall    from(const TrackBall& orig) { TrackBall tb = orig; return tb; }
    TrackBall           update(const TrackBall& prev, const glm::vec2& pos, bool pressed, float width, float height) const;

private:
    TrackBall() : last_(glm::vec2()), current_(glm::vec2()), isOn_(false), wasPressed_(false) {}
    glm::quat getRotation(float width, float height) const;

    static glm::vec3    project(glm::vec2 pos, float width, float height);

    glm::vec2           last_;
    glm::vec2           current_;
    bool                isOn_;
    bool                wasPressed_;
    glm::quat           rotation_;
};
