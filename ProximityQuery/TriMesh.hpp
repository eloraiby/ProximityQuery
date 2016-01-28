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


//
// Note: this is a cache friendly proximity query, given the following guidelines:
//  http://stackoverflow.com/questions/4087280/approximate-cost-to-access-various-caches-and-main-memory
//
//  Core i7 Xeon 5500 Series Data Source Latency(approximate)[Pg. 22]
//  
//  local  L1 CACHE hit, ~4 cycles(2.1 - 1.2 ns)
//  local  L2 CACHE hit, ~10 cycles(5.3 - 3.0 ns)
//  local  L3 CACHE hit, line unshared               ~40 cycles(21.4 - 12.0 ns)
//  local  L3 CACHE hit, shared line in another core ~65 cycles(34.8 - 19.5 ns)
//  local  L3 CACHE hit, modified in another core    ~75 cycles(40.2 - 22.5 ns)
//  
//  remote L3 CACHE(Ref: Fig.1[Pg. 5])        ~100 - 300 cycles(160.7 - 30.0 ns)
//  
//  local  DRAM                                                   ~60 ns
//  remote DRAM                                                  ~100 ns
//
#include <vector>
#include <memory>
#include <cstdint>
#include <limits>

#include <glm/glm/glm.hpp>
#include <glm/glm/gtx/intersect.hpp>


struct Segment {
    Segment(glm::vec3 start, glm::vec3 end) : start_(start), end_(end) {}

    glm::vec3   start() const { return start_; }
    glm::vec3   end() const { return end_; }

    // get the closest point on segment
    static glm::vec3 closestPointOnSegment(const Segment& s, const glm::vec3& pt);

private:
    glm::vec3   start_;
    glm::vec3   end_;
};

struct AABB {
    AABB(glm::vec3 mn, glm::vec3 mx) : min_(mn), max_(mx) {}
    glm::vec3   min() const { return min_; }
    glm::vec3   max() const { return max_; }

    static inline AABB intersection(const AABB& a, const AABB& b) {
        auto aMin = a.min();
        auto aMax = a.max();
        auto bMin = b.min();
        auto bMax = b.max();

        auto mn = glm::max(aMin, bMin);
        auto mx = glm::min(aMax, bMax);
        return AABB(mn, mx);
    }

    static inline bool overlap(const AABB& a, const AABB& b) {
        if (a.max_.x < b.min_.x) return false;
        if (a.max_.y < b.min_.y) return false;
        if (a.max_.z < b.min_.z) return false;

        if (a.min_.x > b.max_.x) return false;
        if (a.min_.y > b.max_.y) return false;
        if (a.min_.z > b.max_.z) return false;

        return true;
    }

    // Classic Graphcis Gems 2
    static bool  intersectSphere(const AABB& bbox, const glm::vec3& center, float radius);
    static void  subdivide(const AABB& bbox, std::vector<AABB>& outBoxes);

private:
    static inline float sqr_(float s) { return s * s; }
    glm::vec3   min_;
    glm::vec3   max_;
};

struct TriMesh {
    typedef std::shared_ptr<TriMesh>    Ptr;

    struct Vertex {
        glm::vec3   position;
        glm::vec3   normal;
        glm::vec4   color;
    };

    struct Tri {
        Vertex    v[3];

        static glm::vec3    closestOnTri(const Tri& tri, const glm::vec3& pt);
        static AABB         boundingBox(const Tri& tri)
        {
            auto v0 = tri.v[0].position;
            auto v1 = tri.v[1].position;
            auto v2 = tri.v[2].position;

            return AABB(min(v0, min(v1, v2)), max(v0, max(v1, v2)));
        }

    };

    TriMesh(const std::vector<Tri>& tris) : tris_(tris)
                                          , bbox_(glm::vec3(std::numeric_limits<float>::max(), std::numeric_limits<float>::max(), std::numeric_limits<float>::max())
                                                , glm::vec3(-std::numeric_limits<float>::max(), -std::numeric_limits<float>::max(), -std::numeric_limits<float>::max()))
    {
        glm::vec3 mn(std::numeric_limits<float>::max(), std::numeric_limits<float>::max(), std::numeric_limits<float>::max());
        glm::vec3 mx(-std::numeric_limits<float>::max(), -std::numeric_limits<float>::max(), -std::numeric_limits<float>::max());

        for (auto t : tris) {
            for (auto v : t.v) {
                mn = glm::min(mn, v.position);
                mx = glm::max(mx, v.position);
            }
        }

        bbox_ = AABB(mn, mx);
    }

    const AABB&     bbox() const { return bbox_; }
    const std::vector<Tri>&     tris() const { return tris_; }

    static glm::vec3    closestOnMesh(TriMesh::Ptr mesh, const glm::vec3& pt);

private:
    std::vector<Tri>    tris_;
    AABB            bbox_;
};

//
// AABBNode : what is going to be called lvariant in C++1z (algeabric data type)
//
struct AABBNode {
    enum class Type {
        NODE,
        LEAF
    };

    const AABB& bbox() const { return bbox_; }
    Type        type() const { return type_; }

    const glm::vec4&    color() const { return color_; }

    struct Node;
    struct Leaf;

protected:
    AABBNode(const AABB& bbox, Type type) : bbox_(bbox), type_(type) {}

    glm::vec4   color_;
    AABB        bbox_;
    Type        type_;
    size_t      index_[8];  // children or leaf index
};

struct AABBNode::Node : public AABBNode {
    Node(const AABB& bbox, size_t children[]) : AABBNode(bbox, Type::NODE) {
        for (size_t i = 0; i < 8; ++i) {
            index_[i] = children[i];
        }
    }

    size_t      operator[] (size_t i) const { return index_[i]; }
};

struct AABBNode::Leaf : public AABBNode {
    Leaf(const AABB& bbox, size_t triMesh, const glm::vec4& color) : AABBNode(bbox, Type::LEAF) {
        color_ = color;
        index_[0] = triMesh;
    }

    size_t     triMesh() const { return index_[0]; }
};

//
// Cache friendly collision mesh: This is done by building a bounding box tree and keeping leaves and nodes separate.
// - the nodes are lightweight structures and the whole table is kept in L1 or L2 cache.
// - only the closest leaf is kept in L1, L2 or L3 cache and nothing else is needed.
// - As such, the processor will keep old volumes, if the point is still close enough
//
struct CollisionMesh {
    typedef std::shared_ptr<CollisionMesh> Ptr;

    size_t                      rootId() const { return rootId_; }
    const std::vector<AABBNode>&      nodes() const { return nodes_; }
    const std::vector<TriMesh::Ptr>&  leaves() const { return leaves_; }

    static Ptr      build(TriMesh::Ptr orig, size_t maxTriCountHint);

private:
    CollisionMesh(size_t rootId, const std::vector<AABBNode>& nodes, const std::vector<TriMesh::Ptr>& leaves) : rootId_(rootId), nodes_(nodes), leaves_(leaves) {}
    size_t                      rootId_;    // given the way it's built right now, it's the last element! this might change however in the future
    std::vector<AABBNode>       nodes_;
    std::vector<TriMesh::Ptr>   leaves_;
};

struct ProximityQuery {
    typedef std::shared_ptr<ProximityQuery> Ptr;

    glm::vec3       closestPointOnMesh(const glm::vec3& pt, float radius, int& leaf) const;

    static Ptr      create(CollisionMesh::Ptr triMesh) { return Ptr(new ProximityQuery(triMesh)); }

private:

    ProximityQuery(CollisionMesh::Ptr mesh) : cm_(mesh) {}
    CollisionMesh::Ptr  cm_;
};
