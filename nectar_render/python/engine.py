import _pathtracer

import tqdm
import numpy as np
from os      import PathLike
from pathlib import Path
from typing  import Self, TypeAlias
from PIL     import Image
from numpy   import ndarray

from nectar_render.python.camera import Camera

Engine: TypeAlias = _pathtracer.engine.RenderEngine

class RenderEngine:
    DEVICE_PTR: int = None
    CAMERA:  Camera = None
    ENGINE:  Engine = None
    
    def __init__(
        self:      Self,
        camera:    Camera = Camera(),
        samples:   int = 10,
        max_depth: int = 8,
        seed:      int | None = None,
        progress_bar:    bool = False
    ) -> None:
        self.__setattr__('CAMERA', camera)
        self.__setattr__('ENGINE', Engine())
        self.ENGINE.on_sample = (
            self._on_sample if progress_bar else lambda *_ : None)
        self.ENGINE.initialize(
            self.CAMERA.cdata, samples, max_depth, 
            seed if seed is not None 
            else np.random.random_integers(0, 999999)
        )
        
    def _on_sample(
        self:        Self, 
        sample:      int, 
        num_samples: int, 
        progress:    float
    ) -> None:
        with tqdm.tqdm(total=num_samples, leave=sample==num_samples) as bar:
            bar.update(sample)

    def render(self: Self) -> None:
        object.__setattr__(self, 'DEVICE_PTR', self.ENGINE.render())
        
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

