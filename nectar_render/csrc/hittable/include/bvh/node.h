#pragma once

#include <algorithm>
#include "hittable/include/bvh/aabb.h"

struct BVHNode {
    AABB bbox;
    int  left;
    int  right;
    int  object;
};

template<typename T>
class BVH {
public:
    std::vector<BVHNode> nodes;
    std::vector<T> items;

    template<typename BBoxFn>
    __host__ void build(std::vector<T> input_items, BBoxFn&& get_bbox) {
        items = std::move(input_items);
        nodes.clear();
        if (items.empty()) return;
        nodes.reserve(items.size() * 2);

        std::vector<AABB> boxes(items.size());
        for (size_t i = 0; i < items.size(); i++) 
            boxes[i] = get_bbox(items[i]);

        std::vector<int> order(items.size());
        std::iota(order.begin(), order.end(), 0);

        build_recursive(order, boxes, 0, (int)order.size());

        std::vector<T> reordered(items.size());
        for (size_t i = 0; i < order.size(); i++) 
            reordered[i] = std::move(items[order[i]]);
        items = std::move(reordered);
    }

private:

    __host__ int build_recursive(
        std::vector<int>& order, 
        std::vector<AABB>& boxes, 
        int start, 
        int end
    ) {
        int idx = (int)nodes.size();
        nodes.push_back({});

        int span = end - start;

        if (span == 1) {
            nodes[idx].object = start;
            nodes[idx].left   = -1;
            nodes[idx].right  = -1;
            nodes[idx].bbox   = boxes[order[start]];
            return idx;
        }

        AABB combined = boxes[order[start]];
        for (int i = start + 1; i < end; i++)
            combined = AABB(combined, boxes[order[i]]);

        float x_len = combined.x.max - combined.x.min;
        float y_len = combined.y.max - combined.y.min;
        float z_len = combined.z.max - combined.z.min;
        int axis = (x_len > y_len && x_len > z_len) 
                 ? 0 : (y_len > z_len ? 1 : 2);

        std::sort(order.begin() + start, order.begin() + end, 
            [&](int a, int b) {
                return boxes[a].axis_interval(axis).min 
                     < boxes[b].axis_interval(axis).min;
            }
        );

        int mid = start + span / 2;
        nodes[idx].object = -1;
        nodes[idx].left   = build_recursive(order, boxes, start, mid);
        nodes[idx].right  = build_recursive(order, boxes, mid, end);
        nodes[idx].bbox   = AABB(
            nodes[nodes[idx].left].bbox, nodes[nodes[idx].right].bbox
        );
        return idx;
    }
};

