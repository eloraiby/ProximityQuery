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
#include "TriMesh.hpp"


using namespace std;
using namespace glm;

// get the closest point on segment
glm::vec3
Segment::closestPointOnSegment(const Segment& s, const glm::vec3& pt) {
    auto dir = s.end_ - s.start_;
    auto dir2pt = pt - s.start_;

    auto d = glm::dot(dir, dir2pt);
    auto n = glm::dot(dir, dir);

    if (d < 0.0f) return s.start_;
    if (d > n) return s.end_;

    auto t = d / n;
    return s.start_ + t * dir;
}

////////////////////////////////////////////////////////////////////////////////

// Classic Graphcis Gems 2 : sphere/box intersection
bool
AABB::intersectSphere(const AABB& bbox, const glm::vec3& center, float radius) {
    auto dist = 0.0f;

    if (center.x < bbox.min_.x) { dist += sqr_(center.x - bbox.min_.x); }
    else if (center.x > bbox.max_.x) { dist += sqr_(center.x - bbox.max_.x); }

    if (center.y < bbox.min_.y) { dist += sqr_(center.y - bbox.min_.y); }
    else if (center.y > bbox.max_.y) { dist += sqr_(center.y - bbox.max_.y); }

    if (center.z < bbox.min_.z) { dist += sqr_(center.z - bbox.min_.z); }
    else if (center.z > bbox.max_.z) { dist += sqr_(center.z - bbox.max_.z); }

    return radius * radius > dist;
}

static ivec3 cubeTable[8] = {
    { 0, 1, 0 },
    { 1, 1, 0 },
    { 1, 1, 1 },
    { 0, 1, 1 },

    { 0, 0, 0 },
    { 1, 0, 0 },
    { 1, 0, 1 },
    { 0, 0, 1 }
};

void
AABB::subdivide(const AABB& bbox, std::vector<AABB>& outBoxes) {
    vec3 ps[2] = { bbox.min(), bbox.max() };

    vec3    vs[8];
    for (size_t i = 0; i < 8; ++i) {
        vs[i] = vec3(ps[cubeTable[i].x][0], ps[cubeTable[i].y][1], ps[cubeTable[i].z][2]);
    }

    auto center = (bbox.max() + bbox.min()) * 0.5f;

    for (size_t i = 0; i < 8; ++i) {
        outBoxes.push_back(AABB(glm::min(center, vs[i]), glm::max(center, vs[i])));
    }
}

////////////////////////////////////////////////////////////////////////////////

///
/// get the closest point on tri
///
glm::vec3
TriMesh::Tri::closestOnTri(const Tri& tri, const glm::vec3& pt) {

    auto v0 = tri.v[0].position;
    auto v1 = tri.v[1].position;
    auto v2 = tri.v[2].position;

    //
    // the good stuff is here :) solve it using coordinate systems
    //

    // center coordinate system around v0
    auto X = v1 - v0;
    auto Y = v2 - v0;
    auto Z = cross(X, Y); // create the 3rd axis
    auto baryCoords = inverse(mat3(X, Y, Z)) * (pt - v0);  // get the barycentric coordinates

    // reject the last component
    auto u = baryCoords.x, v = baryCoords.y;

    if (u > 0.0f && u < 1.0f &&
        v > 0.0f && v < 1.0f &&
        u + v < 1.0f) { // inside triangle!
        return v0 + X * u + Y * v;
    } else {  // out of triangle, fall on segments
        auto ps0 = Segment::closestPointOnSegment(Segment(v0, v1), pt);
        auto ps1 = Segment::closestPointOnSegment(Segment(v1, v2), pt);
        auto ps2 = Segment::closestPointOnSegment(Segment(v2, v0), pt);

        auto l0 = glm::length(pt - ps0);
        auto l1 = glm::length(pt - ps1);
        auto l2 = glm::length(pt - ps2);

        auto minDistance = min(l0, min(l1, l2));

        if (l0 == minDistance) return ps0;
        if (l1 == minDistance) return ps1;
        return ps2;
    }
}



////////////////////////////////////////////////////////////////////////////////
glm::vec3
TriMesh::closestOnMesh(TriMesh::Ptr mesh, const glm::vec3& pt) {
    auto minDistance = std::numeric_limits<float>::max();
    auto minPoint = vec3(std::numeric_limits<float>::max(), std::numeric_limits<float>::max(), std::numeric_limits<float>::max());

    auto tris = mesh->tris();

    // loop through all triangles and find the closest point
    for (auto t : tris) {
        auto mTemp = Tri::closestOnTri(t, pt);
        if (glm::length(pt - mTemp) < minDistance) {
            minPoint = mTemp;
            minDistance = glm::length(pt - mTemp);
        }
    }

    return minPoint;
}

////////////////////////////////////////////////////////////////////////////////
struct BvhTri {
    TriMesh::Tri    tri;
    AABB            box;
    BvhTri(const TriMesh::Tri& t) : tri(t), box(TriMesh::Tri::boundingBox(tri)) {}
};


struct BvhNode {
    typedef shared_ptr<BvhNode> Ptr;

    AABB        box;

    std::vector<BvhTri> tris;   // 0 indicates node, > 0 indicates leaf
    BvhNode::Ptr    children[8];

    BvhNode(const AABB& box, const std::vector<BvhTri>& tris) : box(box), tris(tris) {}
    
    static BvhNode::Ptr    subdivide(const std::vector<BvhTri>& tris, int maxTriCountHint);
};

BvhNode::Ptr
BvhNode::subdivide(const std::vector<BvhTri>& tris, int maxTriCountHint) {
    auto minTs = glm::vec3(std::numeric_limits<float>::max(), std::numeric_limits<float>::max(), std::numeric_limits<float>::max());
    auto maxTs = glm::vec3(-std::numeric_limits<float>::max(), -std::numeric_limits<float>::max(), -std::numeric_limits<float>::max());

    for (auto t : tris) {
        minTs = glm::min(minTs, t.box.min());
        maxTs = glm::max(maxTs, t.box.max());
    }

    auto allTrisBox = AABB(minTs, maxTs);

    if (tris.size() > maxTriCountHint) {    // the tri count still exceeds the max limit hint
        std::vector<AABB> outBoxes;
        AABB::subdivide(allTrisBox, outBoxes);

        //

    } else {
        return Ptr(new BvhNode(allTrisBox, tris));
    }
}


CollisionMesh::Ptr
CollisionMesh::build(TriMesh::Ptr orig) {

    // build the bvh triangles
    std::vector<BvhTri> bvhTris;

    for (auto t : orig->tris()) {
        bvhTris.push_back(BvhTri(t));
    }

    // 
   

    return nullptr;
}
