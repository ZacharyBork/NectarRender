import _pathtracer

import numpy as np
from os      import PathLike
from pathlib import Path
from typing  import Self
from PIL     import Image
from numpy   import ndarray

from nectar_render.python.camera import Camera

class RenderEngine:
    DEVICE_PTR: int = None
    CAMERA:     Camera = None
    
    def __init__(
        self:       Self,
        camera:     Camera = Camera(),
        samples:    int = 10,
        max_depth:  int = 8,
        seed:       int | None = None
    ) -> None:
        object.__setattr__(self, 'CAMERA', camera)
        
        _pathtracer.engine.initialize(
            self.CAMERA.cdata, samples, max_depth, 
            seed if seed is not None else np.random.random_integers(0, 999999)
        )

    def render(self: Self) -> None: 
        object.__setattr__(self, 'DEVICE_PTR', _pathtracer.engine.render())
        
    def get_data(self: Self) -> ndarray:
        data = _pathtracer.host.to_numpy(
            self.DEVICE_PTR, (3,) + self.CAMERA.resolution
        )
        return (data.transpose(1, 2, 0) * 255).astype(np.uint8)
    
    def save_image(self: Self, path: PathLike) -> None:
        path = Path(path).resolve()
        if not path.parent.exists():
            raise FileNotFoundError(
                f'Unable to locate output directory: {path.as_posix()}')
        
        output = self.get_data()
        Image.fromarray(output).save(path)

