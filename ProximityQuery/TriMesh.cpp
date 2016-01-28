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

#include <iostream>

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

    outBoxes.clear();

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

    bool                isLeaf;
    AABB                box;

    std::vector<BvhTri> tris;   // 0 indicates node, > 0 indicates leaf
    std::vector<BvhNode::Ptr>    children;

    BvhNode(bool isLeaf, const AABB& box, const std::vector<BvhTri>& tris) : isLeaf(isLeaf), box(box), tris(tris) {}
    BvhNode(bool isLeaf, const AABB& box, const std::vector<BvhNode::Ptr>& children) : isLeaf(isLeaf), box(box), children(children) {}

    static BvhNode::Ptr subdivide(const std::vector<BvhTri>& tris, size_t maxTriCountHint);

    static size_t       mapToAABBNodes(BvhNode::Ptr node, std::vector<AABBNode>& nodes, std::vector<TriMesh::Ptr>& leaves);
};

BvhNode::Ptr
BvhNode::subdivide(const std::vector<BvhTri>& tris, size_t maxTriCountHint) {
    auto minTs = glm::vec3(std::numeric_limits<float>::max(), std::numeric_limits<float>::max(), std::numeric_limits<float>::max());
    auto maxTs = glm::vec3(-std::numeric_limits<float>::max(), -std::numeric_limits<float>::max(), -std::numeric_limits<float>::max());

    for (auto t : tris) {
        minTs = glm::min(minTs, t.box.min());
        maxTs = glm::max(maxTs, t.box.max());
    }

    auto allTrisBox = AABB(minTs, maxTs);

    if (tris.size() > maxTriCountHint) {    // the tri count still exceeds the max limit hint
        vector<AABB> outBoxes;
        AABB::subdivide(allTrisBox, outBoxes);

        // 1st pass - count the number of triangles included in each box,
        // if any box intersects all triangles then we have reached the limit and allTrisBox is a leaf
        size_t tCount[8] = { 0 };
        for (auto t : tris) {
            for (size_t i = 0; i < outBoxes.size(); ++i ) {
                if (AABB::overlap(outBoxes[i], t.box)) ++tCount[i];
            }
        }

        for (auto tc : tCount) {
            if (tc == tris.size()) {    // one of them has all the triangles, bail!
                return Ptr(new BvhNode(true, allTrisBox, tris));
            }
        }

        // 2nd pass - sort the triangles into their respective boxes
        // rule: one triangle can belong to only one box
        vector<BvhTri> boxTris[8];

        for (auto t : tris) {
            if (AABB::overlap(outBoxes[0], t.box)) boxTris[0].push_back(t);
            else if (AABB::overlap(outBoxes[1], t.box)) boxTris[1].push_back(t);
            else if (AABB::overlap(outBoxes[2], t.box)) boxTris[2].push_back(t);
            else if (AABB::overlap(outBoxes[3], t.box)) boxTris[3].push_back(t);
            else if (AABB::overlap(outBoxes[4], t.box)) boxTris[4].push_back(t);
            else if (AABB::overlap(outBoxes[5], t.box)) boxTris[5].push_back(t);
            else if (AABB::overlap(outBoxes[6], t.box)) boxTris[6].push_back(t);
            else if (AABB::overlap(outBoxes[7], t.box)) boxTris[7].push_back(t);
        }

        // 3rd pass - build the node recursively
        vector<Ptr> children;
        for (size_t i = 0; i < 8; ++i) {
            children.push_back(subdivide(boxTris[i], maxTriCountHint));
        }

        return Ptr(new BvhNode(false, allTrisBox, children));
    } else {
        return Ptr(new BvhNode(true, allTrisBox, tris));
    }
}

float
frand() {
    return (float(rand() & 0xFFFF) / float(0x10000));
}

size_t
BvhNode::mapToAABBNodes(BvhNode::Ptr node, std::vector<AABBNode>& nodes, std::vector<TriMesh::Ptr>& leaves) {
    if (node->isLeaf) {
        vector<TriMesh::Tri> tris;
        auto color = vec4(frand(), frand(), frand(), 0.0f);
        for (auto bt : node->tris) {
            TriMesh::Tri tmp = bt.tri;
            tmp.v[0].color = tmp.v[1].color = tmp.v[2].color = color;   // for debugging purposes
            tris.push_back(tmp);
        }

        leaves.push_back(TriMesh::Ptr(new TriMesh(tris)));
        nodes.push_back(AABBNode::Leaf(node->box, leaves.size() - 1, color));
    } else {
        int idx = 0;
        size_t bIds[8] = { 0 };
        for (auto ch : node->children) {
            bIds[idx] = mapToAABBNodes(ch, nodes, leaves);
            ++idx;
        }
        nodes.push_back(AABBNode::Node(node->box, bIds));
    }
    return nodes.size() - 1;
}

////////////////////////////////////////////////////////////////////////////////

CollisionMesh::Ptr
CollisionMesh::build(TriMesh::Ptr orig, size_t maxTriCountHint) {

    // build the bvh triangles
    std::vector<BvhTri> bvhTris;

    for (auto t : orig->tris()) {
        bvhTris.push_back(BvhTri(t));
    }

    // build the root node
    auto root = BvhNode::subdivide(bvhTris, maxTriCountHint);

    // collect the leaves
    vector<AABBNode> nodes;
    vector<TriMesh::Ptr> leaves;
    size_t rootId = BvhNode::mapToAABBNodes(root, nodes, leaves);
 
    return Ptr(new CollisionMesh(rootId, nodes, leaves));
}
