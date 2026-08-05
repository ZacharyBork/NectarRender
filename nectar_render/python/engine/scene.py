import _pathtracer
root = _pathtracer.scene

from typing import Self, TypeAlias
from collections.abc import Sequence

from nectar_render.python.hittable      import Hittable
from nectar_render.python.engine.lights import Skylight

SceneInterface: TypeAlias = root.SceneInterface

class Scene(root.Scene):
    def __init__(
        self:      Self, 
        hittables: Sequence[Hittable], 
        lights:    Sequence[Hittable] = [],
        skylight:  Skylight | None = None
    ) -> None:
        if len(hittables) == 0:
            raise ValueError(f'Hittables list cannot be empty.')
            
        skylight = Skylight() if skylight is None else skylight
        super().__init__(hittables, lights, skylight)




