from __future__ import annotations
from typing     import TYPE_CHECKING
if TYPE_CHECKING:
    from nectar_render import Hittable
    from nectar_render.python.engine import SkyLight

import _pathtracer
root = _pathtracer.engine

from typing import Self
from collections.abc import Sequence

class Scene(root.Scene):
    def __init__(
        self:      Self, 
        hittables: Sequence[Hittable], 
        skylight:  SkyLight | None = None
    ) -> None:
        if len(hittables) == 0:
            raise ValueError(f'Hittables list cannot be empty.')
            
        skylight = SkyLight.black() if skylight is None else skylight
        super().__init__(hittables, skylight)




