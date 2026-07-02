import _pathtracer
root = _pathtracer.engine

import time
import ctypes
import numpy as np
from os      import PathLike
from pathlib import Path
from typing  import Self, TypeAlias
from PIL     import Image
from numpy   import ndarray

from nectar_render.python import core, progress
from nectar_render.python.engine.scene   import Scene
from nectar_render.python.engine.camera  import Camera
from nectar_render.python.engine.denoise import Denoiser, TVDenoiser

Engine:     TypeAlias = root.RenderEngine
Transform:  TypeAlias = root.Transform
SampleMode: TypeAlias = root.SampleMode

class RenderEngine:
    ENGINE: Engine = None
    SILENT:   bool = False
        
    def __init__(
        self:      Self,
        camera:    Camera = Camera(),
        max_depth: int = 8,
        seed:      int | None = None,
        silent:    bool = False
    ) -> None:        
        self.__setattr__('SILENT', silent)
        self.__setattr__('ENGINE', Engine(
            camera, max_depth, 
            seed if seed is not None 
            else np.random.random_integers(0, 999999)
        ))
        self.ENGINE.on_frame_finished = self.on_frame_finished

    def log(self: Self, log_string: str) -> None:
        if not self.SILENT: print(log_string)
        
    def on_frame_finished(self: Self, frame_idx: int) -> None:
        if not self.SILENT:
            progress.pbar('render', self.num_samples).update(frame_idx)
        
    def sample(
        self:  Self, 
        scene: Scene, 
        mode:  SampleMode = SampleMode.ACCUMULATE
    ) -> Self:
        self.ENGINE.sample(scene, mode)
        return self
            
    def render(
        self:    Self, 
        scene:   Scene, 
        samples: int = 100,
        mode:    SampleMode = SampleMode.ACCUMULATE
    ) -> Self:
        self.num_samples = samples
        start = time.time()
        self.ENGINE.render(scene, samples, mode)
        core.cuda_synchronize()
        self.log(f'Render complete. Time taken: {(time.time() - start):.4f}')
        return self
    
    def denoise(self: Self, denoiser: Denoiser = TVDenoiser()) -> Self:
        layers = self.ENGINE.layers()
        denoiser.run(layers.beauty)
        return self

    def get_data(self: Self) -> ndarray:
        layers  = self.ENGINE.layers()
        beauty  = layers.beauty
        pinned  = beauty.is_pinned()
        C, H, W = beauty.shape()
        
        beauty.tonemap(1.0)
        # beauty.linear_to_gamma()
        
        if not pinned: arr = beauty.numpy()
        else:
            ptr = beauty.readback_pinned()
            arr = np.ctypeslib.as_array(
                (ctypes.c_float * beauty.n_elements()).from_address(ptr)
            ).copy()
            
        image = (arr.reshape(C, H, W).transpose(1, 2, 0) * 255)
        return image.astype(np.uint8)

    def save_image(self: Self, path: PathLike) -> Self:
        path = Path(path).resolve()
        if not path.parent.exists():
            raise FileNotFoundError(
                f'Unable to locate output directory: {path.as_posix()}')
        
        output = self.get_data()
        Image.fromarray(output).save(path)
        return self

