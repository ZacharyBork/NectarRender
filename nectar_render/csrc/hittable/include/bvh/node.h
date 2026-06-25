#pragma once

#include <algorithm>
#include "hittable/include/hittable/hittable.h"

struct BVHNode {
    AABB bbox;
    int  left;
    int  right;
    int  object;
};

class BVH {
public:
    std::vector<BVHNode>   nodes;
    std::vector<Hittable*> objects;

    __host__ BVH() {}

    void build(std::vector<Hittable*>& objs) {
        objects = objs;
        nodes.reserve(objs.size() * 2);
        build_recursive(objs, 0, objs.size());
    }

private:

    int build_recursive(std::vector<Hittable*>& objs, int start, int end) {
        int idx = nodes.size();
        nodes.push_back({});

        size_t span = end - start;

        if (span == 1) {
            nodes[idx].object = start;
            nodes[idx].left   = -1;
            nodes[idx].right  = -1;
            nodes[idx].bbox   = objs[start]->build_bbox();
            return idx;
        }

        AABB combined = objs[start]->build_bbox();
        for (int i = start + 1; i < end; i++)
            combined = AABB(combined, objs[i]->build_bbox());

        float x_len = combined.x.max - combined.x.min;
        float y_len = combined.y.max - combined.y.min;
        float z_len = combined.z.max - combined.z.min;
        int axis = (x_len>y_len && x_len>z_len) ? 0 : (y_len>z_len) ? 1 : 2;

        auto comparator = (axis == 0) ? box_x_compare
                        : (axis == 1) ? box_y_compare
                                      : box_z_compare;

        std::sort(objs.begin() + start, objs.begin() + end, comparator);
        int mid = start + span / 2;

        nodes[idx].object = -1;
        nodes[idx].left   = build_recursive(objs, start, mid);
        nodes[idx].right  = build_recursive(objs, mid, end);
        nodes[idx].bbox   = AABB(
            nodes[nodes[idx].left].bbox,
            nodes[nodes[idx].right].bbox
        );
        return idx;
    }

    static bool box_compare(const Hittable* a, const Hittable* b, int axis) {
        return a->build_bbox().axis_interval(axis).min 
             < b->build_bbox().axis_interval(axis).min;
    }
    static bool box_x_compare(const Hittable* a, const Hittable* b) { 
        return box_compare(a, b, 0); 
    }
    static bool box_y_compare(const Hittable* a, const Hittable* b) { 
        return box_compare(a, b, 1); 
    }
    static bool box_z_compare(const Hittable* a, const Hittable* b) { 
        return box_compare(a, b, 2); 
    }
};
