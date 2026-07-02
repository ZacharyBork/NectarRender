from os      import PathLike
from pathlib import Path
from typing  import Self

from nectar_render.python import (
    RenderEngine, Scene, Camera, Hittable, Vector3, Color, Material
)

class CornellBox(RenderEngine):
    skylight = None
    camera   = Camera(
        resolution   = (1024, 1024),
        position     = (0.0, 0.0, 2.0),
        rotation     = (0.0, 0.0, 0.0),
        focal_length = 3.0
    )
    
    hittables = [
        Hittable.QUAD( # Light
            Vector3(0.0, 0.499, 0.0),
            Vector3(0.0, 0.0, 0.0),
            Vector3(0.2),
            Material.EMISSIVE(Color(35.0, 35.0, 35.0))
        ),
        Hittable.QUAD( # Bottom
            Vector3(0.0, -0.5, 0.0),
            Vector3(0.0, 0.0, 0.0),
            Vector3(1.0),
            Material.LAMBERTIAN(Color.white())
        ),
        Hittable.QUAD( # Top
            Vector3(0.0, 0.5, 0.0),
            Vector3(0.0, 180.0, 0.0),
            Vector3(1.0),
            Material.LAMBERTIAN(Color.white())
        ),
        Hittable.QUAD( # Right
            Vector3(0.5, 0.0, 0.0),
            Vector3(0.0, 0.0, 90.0),
            Vector3(1.0),
            Material.LAMBERTIAN(Color.red())
        ),
        Hittable.QUAD( # Left
            Vector3(-0.5, 0.0, 0.0),
            Vector3(0.0, 0.0, -90.0),
            Vector3(1.0),
            Material.LAMBERTIAN(Color.green())
        ),
        Hittable.QUAD( # Back
            Vector3(0.0, 0.0, -0.5),
            Vector3(-90.0, 0.0, 0.0),
            Vector3(1.0),
            Material.LAMBERTIAN(Color.white())
        ),
        
        Hittable.CUBE(
            Vector3(0.2, -0.35, 0.2), 
            Vector3(0.0, 35.0, 0.0),
            Vector3(0.3),
            Material.LAMBERTIAN(Color.white())
        ),
        Hittable.CUBE(
            Vector3(-0.2, -0.2, -0.1), 
            Vector3(0.0, 35.0, 0.0),
            Vector3(0.3, 0.6, 0.3),
            Material.LAMBERTIAN(Color.white())
        ),
    ]
  
    def __init__(
        self:      Self, 
        samples:   int = 10,
        max_depth: int = 8,
        seed:      int | None = None,
        silent:    bool = False
    ) -> None:
        super().__init__(
            camera    = self.camera,
            samples   = samples,
            max_depth = max_depth,
            seed      = seed,
            silent    = silent
        )
        
    def render(self: Self) -> Self:
        return super().render(Scene(self.hittables, self.skylight))

