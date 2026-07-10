#include "engine/include/engine/camera.h"

__global__ void update_device_camera_kernel(
    DeviceCamera* d_cam,
    const Vector3& delta_position,
    const Vector3& delta_rotation
) {
    d_cam->position += delta_position;
    d_cam->rotation = d_cam->rotation 
                    * rotation_from_euler(deg2rad(delta_rotation));
}

void update_device_camera(
    DeviceCamera* d_cam,
    const Vector3& delta_position,
    const Vector3& delta_rotation
) {
    update_device_camera_kernel<<<1, 1>>>(
        d_cam, delta_position, delta_rotation
    );
}


