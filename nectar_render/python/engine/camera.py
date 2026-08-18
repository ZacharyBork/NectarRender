import _pathtracer
root = _pathtracer.engine.camera

from typing import Self, TypeAlias
from nectar_render.python.core import Vector2, Vector3

CameraParams: TypeAlias = root.CameraParams

class Camera(root.Camera):    
    def __init__(
        self:       Self,
        resolution: tuple[int, int] = (512, 512),
        position:   tuple[float, float, float] = (0.0, 0.0, 0.0),
        rotation:   tuple[float, float, float] = (0.0, 0.0, 0.0),
        num_samples:      int = 512,
        focal_length:   float = 5.0,
        focus_distance: float = 10.0,
        aperture:       float = 0.01,
        sensor_width:   float = 2.0,
        shutter_speed:  float = 1.0
    ) -> None:
        params = root.CameraParams(
            Vector2(*resolution), Vector3(*position), Vector3(*rotation), 
            num_samples, focal_length, focus_distance, aperture, sensor_width, 
            shutter_speed
        )
        super().__init__(params)


