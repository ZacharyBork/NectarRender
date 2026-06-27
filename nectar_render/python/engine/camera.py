import _pathtracer
root = _pathtracer.engine.camera

from typing import Self, TypeAlias

CameraParams: TypeAlias = root.CameraParams

class Camera(root.Camera):    
    def __init__(
        self:       Self,
        resolution: tuple[int, int] = (512, 512),
        position:   tuple[float, float, float] = (0.0, 0.0, 0.0),
        rotation:   tuple[float, float, float] = (0.0, 0.0, 0.0),
        focal_length:   float = 5.0,
        focus_distance: float = 10.0,
        aperture:       float = 0.01,
        sensor_width:   float = 2.0,
        shutter_speed:  float = 1.0
    ) -> None:
        super().__init__(CameraParams(*[
            v for k, v in locals().items() 
            if not k in ['__class__', 'self']]
        ))

    @property
    def params(self: Self) -> CameraParams: return self.PARAMS

