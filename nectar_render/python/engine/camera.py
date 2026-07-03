import _pathtracer
root = _pathtracer.engine.camera

from typing import Self

class Camera(root.Camera):    
    def __init__(
        self:       Self,
        resolution: tuple[int, int] = (512, 512),
        position:   tuple[float, float, float] = (0.0, 0.0, 0.0),
        rotation:   tuple[float, float, float] = (0.0, 0.0, 0.0),
        num_samples:      int = 500,
        focal_length:   float = 5.0,
        focus_distance: float = 10.0,
        aperture:       float = 0.01,
        sensor_width:   float = 2.0,
        shutter_speed:  float = 1.0
    ) -> None:
        super().__init__(
            resolution, position, rotation, num_samples, focal_length,
            focus_distance, aperture, sensor_width, shutter_speed
        )
        
        # super().__init__(*[
        #     v for k, v in locals().items() 
        #     if not k in ['__class__', 'self']]
        # )
