#pragma once

#include <algorithm>
#include "hittable/include/bvh/aabb.h"

// ============================================================================
// DATA STRUCTURES
// ============================================================================

struct SAHBin {
    AABB    bbox;
    int32_t count = 0;
};

struct BVHNode {
    AABB    bbox;
    int32_t left;
    int32_t right;
    int32_t object;
};

// ============================================================================
// BVH CLASS DEFINITION
// ============================================================================

template<typename T>
class BVH {
public:

    static constexpr int32_t SAH_BIN_COUNT = 16;

    std::vector<BVHNode> nodes;
    std::vector<T> items;

    template<typename BBoxFn>
    __host__ void build(std::vector<T> input_items, BBoxFn&& get_bbox) {
        items = std::move(input_items);
        nodes.clear(); order.clear(); boxes.clear();
        if (items.empty()) return;
        nodes.reserve(items.size() * 2UL);
        
        boxes.resize(items.size());
        for (size_t i = 0UL; i < items.size(); i++) 
            boxes[i] = get_bbox(items[i]);

        order.resize(items.size());
        std::iota(order.begin(), order.end(), 0);
        
        build_recursive(0, (int32_t)order.size());

        std::vector<T> reordered(items.size());
        for (size_t i = 0UL; i < order.size(); i++) 
            reordered[i] = std::move(items[order[i]]);
        items = std::move(reordered);
    }

private:

    std::vector<int32_t> order{};
    std::vector<AABB>    boxes{};

    __host__ void bbox_from_centroids(
        Vector3& centroid_min,
        Vector3& centroid_max,
        int32_t start, int32_t end
    );

    __host__ void build_bins(
        SAHBin (&bins)[SAH_BIN_COUNT],
        float c_min, float extent, int32_t start, int32_t end, int32_t axis
    );

    __host__ void evaluate_bins(
        SAHBin (&bins)[SAH_BIN_COUNT],
        float&   best_cost,
        int32_t& best_axis,
        int32_t& best_bin,
        int32_t  axis
    );

    __host__ void median_split(
        int32_t start, int32_t end, int32_t axis, size_t idx
    );

    __host__ void partition_order(
        int32_t  start, 
        int32_t  end, 
        size_t   idx,
        int32_t  axis,
        Vector3& centroid_min,
        Vector3& centroid_max,
        int32_t& best_bin
    );

    __host__ int32_t build_recursive(int32_t start, int32_t end);
};

// ============================================================================
// UTILITIES
// ============================================================================

template<typename T>
__host__ inline void BVH<T>::bbox_from_centroids(
    Vector3& centroid_min,
    Vector3& centroid_max,
    int32_t start, int32_t end
) {
    for (int32_t i = start; i < end; i++) {
        const AABB& b = boxes[order[i]];
        Vector3 c(
            (b.x.min + b.x.max) * 0.5f, 
            (b.y.min + b.y.max) * 0.5f, 
            (b.z.min + b.z.max) * 0.5f
        );
        centroid_min = Vector3(
            fminf(centroid_min.x(), c.x()), 
            fminf(centroid_min.y(), c.y()), 
            fminf(centroid_min.z(), c.z())
        );
        centroid_max = Vector3(
            fmaxf(centroid_max.x(), c.x()), 
            fmaxf(centroid_max.y(), c.y()), 
            fmaxf(centroid_max.z(), c.z())
        );
    }
}

template<typename T>
__host__ inline void BVH<T>::build_bins(
    SAHBin (&bins)[SAH_BIN_COUNT],
    float c_min,
    float extent,
    int32_t start, int32_t end, int32_t axis
) {
    for (int32_t i = start; i < end; i++) {
        const AABB& b = boxes[order[i]];
        float imin = b.axis_interval(axis).min;
        float imax = b.axis_interval(axis).max;
        float c = (imin + imax) * 0.5f;
        
        int32_t bin = (int32_t)(SAH_BIN_COUNT * (c - c_min) / extent);
        bin = bin < 0 ? 0 : (bin >= SAH_BIN_COUNT ? SAH_BIN_COUNT-1 : bin);

        bins[bin].count++;
        bins[bin].bbox = bins[bin].count==1 ? b : AABB(bins[bin].bbox, b);
    }
}

template<typename T>
__host__ inline void BVH<T>::evaluate_bins(
    SAHBin (&bins)[SAH_BIN_COUNT],
    float&   best_cost,
    int32_t& best_axis,
    int32_t& best_bin,
    int32_t  axis
) {
    AABB    left_boxes[SAH_BIN_COUNT];
    int32_t left_counts[SAH_BIN_COUNT];
    AABB    right_boxes[SAH_BIN_COUNT];
    int32_t right_counts[SAH_BIN_COUNT];

    AABB running; int32_t running_count = 0; bool started = false;
    for (int32_t i = 0; i < SAH_BIN_COUNT; i++) {
        if (bins[i].count > 0) {
            running = started ? AABB(running, bins[i].bbox) : bins[i].bbox;
            started = true;
        }
        running_count += bins[i].count;
        left_boxes[i] = running;
        left_counts[i] = running_count;
    }

    running_count = 0; started = false;
    for (int32_t i = SAH_BIN_COUNT - 1; i >= 0; i--) {
        if (bins[i].count > 0) {
            running = started ? AABB(running, bins[i].bbox) : bins[i].bbox;
            started = true;
        }
        running_count += bins[i].count;
        right_boxes[i] = running;
        right_counts[i] = running_count;
    }

    for (int32_t i = 0; i < SAH_BIN_COUNT - 1; i++) {
        int32_t n_left  = left_counts[i];
        int32_t n_right = right_counts[i + 1];
        if (n_left == 0 || n_right == 0) continue;

        float area_left  = left_boxes[i].surface_area();
        float area_right = right_boxes[i + 1].surface_area();
        float cost = area_left * n_left + area_right * n_right;

        if (cost < best_cost) {
            best_cost = cost;
            best_axis = axis;
            best_bin  = i;
        }
    }
}

template<typename T>
__host__ inline void BVH<T>::median_split(
    int32_t start, 
    int32_t end, 
    int32_t axis,
    size_t  idx
) {
    std::sort(order.begin() + start, order.begin() + end,
        [&](int32_t a, int32_t b) {
            return boxes[a].axis_interval(axis).min 
                    < boxes[b].axis_interval(axis).min;
        }
    );
    int32_t mid = start + (end - start) / 2;
    nodes[idx].object = -1;
    nodes[idx].left   = build_recursive(start, mid);
    nodes[idx].right  = build_recursive(mid, end);
    nodes[idx].bbox   = AABB(
        nodes[nodes[idx].left].bbox, 
        nodes[nodes[idx].right].bbox
    );
}

template<typename T>
__host__ inline void BVH<T>::partition_order(
    int32_t  start, 
    int32_t  end, 
    size_t   idx,
    int32_t  axis,
    Vector3& centroid_min,
    Vector3& centroid_max,
    int32_t& best_bin
) {
    float c_min  = centroid_min[axis];
    float extent = centroid_max[axis] - c_min;

    auto mid_it = std::partition(order.begin()+start, order.begin()+end,
        [&](int32_t a) {
            const AABB& b = boxes[a];
            float c = (
                b.axis_interval(axis).min + b.axis_interval(axis).max
            ) * 0.5f;

            int32_t bin = (int32_t)(SAH_BIN_COUNT * (c - c_min) / extent);
            bin = bin < 0 ? 0 : (
                bin >= SAH_BIN_COUNT ? SAH_BIN_COUNT - 1 : bin
            );

            return bin <= best_bin;
        }
    );
    int32_t mid = (int32_t)(mid_it - order.begin());

    nodes[idx].object = -1;
    nodes[idx].left   = build_recursive(start, mid);
    nodes[idx].right  = build_recursive(mid, end);
    nodes[idx].bbox   = AABB(
        nodes[nodes[idx].left].bbox, nodes[nodes[idx].right].bbox
    );
}

// ============================================================================
// BVH TREE BUILD
// ============================================================================

template<typename T>
__host__ inline int32_t BVH<T>::build_recursive(int32_t start, int32_t end) {
    int32_t span   = end - start;
    size_t idx = nodes.size();
    nodes.push_back({});
    
    if (span == 1) {
        nodes[idx].object = start;
        nodes[idx].left   = -1;
        nodes[idx].right  = -1;
        nodes[idx].bbox   = boxes[order[start]];
        return idx;
    }

    Vector3 centroid_min( FMAX,  FMAX,  FMAX);
    Vector3 centroid_max(-FMAX, -FMAX, -FMAX);
    bbox_from_centroids(centroid_min, centroid_max, start, end);

    float   best_cost = FMAX;
    int32_t best_axis = -1;
    int32_t best_bin  = -1;

    for (int32_t axis = 0; axis < 3; axis++) {
        float c_min = centroid_min[axis];
        float c_max = centroid_max[axis];
        float extent = c_max - c_min;
        if (extent < EPS) continue;

        SAHBin bins[SAH_BIN_COUNT];

        build_bins(bins, c_min, extent, start, end, axis);
        evaluate_bins(bins, best_cost, best_axis, best_bin, axis);
    }

    if (best_axis == -1) {
        AABB combined = boxes[order[start]];
        for (int32_t i = start + 1; i < end; i++)
            combined = AABB(combined, boxes[order[i]]);

        best_axis = 0;
        float lx = combined.x.max - combined.x.min;
        float ly = combined.y.max - combined.y.min;
        float lz = combined.z.max - combined.z.min;
        best_axis = (lx > ly && lx > lz) ? 0 : (ly > lz ? 1 : 2);
    }

    if (best_bin==-1) { 
        median_split(start, end, best_axis, idx); return idx; 
    }

    partition_order(
        start, end, idx, best_axis, centroid_min, centroid_max, best_bin
    );
    return idx;
}





