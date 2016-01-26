#pragma once

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
#include <cfloat>

#include <glm/glm/glm.hpp>
#include <glm/glm/gtx/intersect.hpp>


struct Segment {
    Segment(glm::vec3 start, glm::vec3 end) : start_(start), end_(end) {}

    glm::vec3   start() const { return start_; }
    glm::vec3   end() const { return end_; }

    // get the closest point on segment
    static inline glm::vec3 closestPointOnSegment(const Segment& s, const glm::vec3& pt) {
        auto dir = s.end_ - s.start_;
        auto dir2pt = pt - s.start_;

        auto d = glm::dot(dir, dir2pt);
        auto n = glm::dot(dir, dir);

        if (d < 0.0f) return s.start_;
        if (d > n) return s.end_;

        auto t = d / n;
        return s.start_ + t * dir;
    }

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

    // Classic Graphcis Gems 2
    static inline bool  intersectSphere(const AABB& bbox, const glm::vec3& center, float radius) {
        auto dist = 0.0f;

        if (center.x < bbox.min_.x) { dist += sqr_(center.x - bbox.min_.x); }
        else if (center.x > bbox.max_.x) { dist += sqr_(center.x - bbox.max_.x); }

        if (center.y < bbox.min_.y) { dist += sqr_(center.y - bbox.min_.y); }
        else if (center.y > bbox.max_.y) { dist += sqr_(center.y - bbox.max_.y); }

        if (center.z < bbox.min_.z) { dist += sqr_(center.z - bbox.min_.z); }
        else if (center.z > bbox.max_.z) { dist += sqr_(center.z - bbox.max_.z); }

        return radius * radius > dist;
    }

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
    };

    TriMesh(const std::vector<Tri>& tris) : tris_(tris)
                                          , bbox_(glm::vec3(FLT_MAX, FLT_MAX, FLT_MAX), glm::vec3(-FLT_MAX, -FLT_MAX, -FLT_MAX))
    {
        glm::vec3 mn(FLT_MAX, FLT_MAX, FLT_MAX);
        glm::vec3 mx(-FLT_MAX, -FLT_MAX, -FLT_MAX);

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

    struct Node;
    struct Leaf;

protected:
    AABBNode(const AABB& bbox, Type type) : bbox_(bbox), type_(type) {}

    AABB        bbox_;
    Type        type_;
    size_t      index_[8];  // children or leaf index
};

struct AABBNode::Node : public AABBNode {
    Node(const AABB& bbox, size_t children[]) : AABBNode(bbox, Type::LEAF) {
        for (size_t i = 0; i < 8; ++i) {
            index_[i] = children[i];
        }
    }

    size_t      operator[] (size_t i) const { return index_[i]; }
};

struct AABBNode::Leaf : public AABBNode {
    Leaf(const AABB& bbox, size_t triMesh) : AABBNode(bbox, Type::NODE) {
        index_[0] = triMesh;
    }

    size_t     triMesh() const { return index_[0]; }
};

//
// Cache friendly collision mesh: This is done by building a bounding box tree and keeping leaves and nodes separate.
// - the nodes are lightweight structures and the whole table is kept in L1 or L2 cache.
// - only the closest leaf is kept in L1, L2 or L3 cache and nothing else is needed.
//
struct CollisionMesh {
    typedef std::shared_ptr<CollisionMesh> Ptr;

    CollisionMesh(const std::vector<AABBNode>& nodes, const std::vector<TriMesh::Ptr>& leaves) {}

    const std::vector<AABBNode>&      nodes() const { return nodes_; }
    const std::vector<TriMesh::Ptr>&  leaves() const { return leaves_; }

private:
    std::vector<AABBNode>       nodes_;
    std::vector<TriMesh::Ptr>   leaves_;
};

struct ProximityQuery {

    struct Result {

    private:
        size_t      node_;
        size_t      leaf_;

        friend struct ProximityQuery;
    };

private:
    CollisionMesh::Ptr  cm_;
};
