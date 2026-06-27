import _pathtracer
root = _pathtracer.engine

import time
import numpy as np
from os      import PathLike
from pathlib import Path
from typing  import Self, TypeAlias
from PIL     import Image
from numpy   import ndarray
from collections.abc import Sequence

from nectar_render.python import core
from nectar_render.python.engine.scene  import Scene
from nectar_render.python.engine.camera import Camera

Transform: TypeAlias = root.Transform
Engine:    TypeAlias = root.RenderEngine

class RenderEngine:
    DEVICE_PTR: int = None
    CAMERA:  Camera = None
    ENGINE:  Engine = None
    SILENT:    bool = False 
    
    def __init__(
        self:      Self,
        camera:    Camera = Camera(),
        samples:   int = 10,
        max_depth: int = 8,
        seed:      int | None = None,
        silent:    bool = False
    ) -> None:
        self.__setattr__('CAMERA', camera)
        self.__setattr__('ENGINE', Engine())
        self.__setattr__('SILENT', silent)
        self.ENGINE.on_frame_finished = self.on_frame_finished
        self.ENGINE.initialize(
            self.CAMERA.cdata, samples, max_depth, 
            seed if seed is not None 
            else np.random.random_integers(0, 999999)
        )
        
    def log(self: Self, log_string: str) -> None:
        if not self.SILENT: print(log_string)
        
    def on_frame_finished(self: Self, frame_idx: int) -> None:
        pass

    def render(self: Self, scene: Scene) -> None:
        start = time.time()
        self.ENGINE.render(scene)
        core.cuda_synchronize()
        self.log(f'Render complete. Time taken: {(time.time() - start):.4f}')
        
    def get_data(self: Self) -> ndarray:
        layers = self.ENGINE.get_render_layers()
        beauty = layers.beauty.numpy()
        return (beauty.transpose(1, 2, 0) * 255).astype(np.uint8)
    
    def save_image(self: Self, path: PathLike) -> None:
        path = Path(path).resolve()
        if not path.parent.exists():
            raise FileNotFoundError(
                f'Unable to locate output directory: {path.as_posix()}')
        
        output = self.get_data()
        Image.fromarray(output).save(path)

