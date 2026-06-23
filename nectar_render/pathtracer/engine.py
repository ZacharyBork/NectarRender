import _pathtracer

import numpy as np
from os      import PathLike
from pathlib import Path
from typing  import Self
from PIL     import Image
from numpy   import ndarray

class RenderEngine:
    DEVICE_PTR:   int = None
    OUTPUT_SHAPE: tuple[int, int, int] = None
    
    def init(
        self:       Self,
        channels:   int = 3, 
        resolution: tuple[int, int] = (512, 512),
        seed:       int | None = None
    ) -> None:
        object.__setattr__(self, 'OUTPUT_SHAPE', (channels,) + resolution)
        
        seed = (
            seed if seed is not None 
            else np.random.random_integers(0, 999999)
        )
        _pathtracer.engine.initialize(self.OUTPUT_SHAPE, seed)

    def render(self: Self) -> None: 
        object.__setattr__(self, 'DEVICE_PTR', _pathtracer.engine.render())
        
    def get_data(self: Self) -> ndarray:
        data = _pathtracer.host.to_numpy(self.DEVICE_PTR, self.OUTPUT_SHAPE)
        return (data.transpose(1, 2, 0) * 255).astype(np.uint8)
    
    def save_image(self: Self, path: PathLike) -> None:
        path = Path(path).resolve()
        if not path.parent.exists():
            raise FileNotFoundError(
                f'Unable to locate output directory: {path.as_posix()}')
        
        output = self.get_data()
        Image.fromarray(output).save(path)

ENGINE = RenderEngine()
