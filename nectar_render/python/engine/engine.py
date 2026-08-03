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
from collections.abc import Callable

from nectar_render.python import core, progress
from nectar_render.python.engine.scene   import Scene
from nectar_render.python.engine.camera  import Camera
from nectar_render.python.engine.denoise import Denoiser, TVDenoiser

Engine:             TypeAlias = root.RenderEngine
EngineType:         TypeAlias = root.EngineType
EnginePollResponse: TypeAlias = root.EnginePollResponse

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

    ### PROPERTIES ###

    @property
    def n_samples(self: Self) -> int:
        return self.ENGINE.n_samples()
    
    @property
    def camera(self: Self) -> Camera: return self.ENGINE.camera()

    ### UTILITIES ###

    def log(self: Self, log_string: str) -> None:
        if not self.SILENT: print(log_string)
        
    ### HOOKS ###
        
    def on_frame_finished(self: Self, frame_idx: int) -> None:
        if not self.SILENT:
            progress.pbar('render', self.n_samples).update(frame_idx)
        
    ### UTILITIES ###
    
    def reset       (self: Self) -> None: self.ENGINE.reset()
    def request_stop(self: Self) -> None: self.ENGINE.request_stop()

    def is_idle     (self: Self) -> bool: return self.ENGINE.is_idle()
    def is_rendering(self: Self) -> bool: return self.ENGINE.is_rendering()
    
    def set_scene(self: Self, scene: Scene) -> None:
        self.ENGINE.set_scene(scene)

    ### RENDERING ###
        
    def render(self: Self) -> Self:
        start = time.time()
        self.ENGINE.render()
        core.cuda_synchronize()
        self.log(f'Render complete. Time taken: {(time.time() - start):.4f}')
        return self
    
    ### DENOISING ###
    
    def denoise(self: Self, denoiser: Denoiser = TVDenoiser()) -> Self:
        layers = self.ENGINE.layers()
        denoiser.run(layers.beauty)
        return self

    ### DATA UTILITIES ###

    def get_data(self: Self) -> ndarray:
        stream = self.ENGINE.stream()
        ptr = stream.readback()
        arr = np.ctypeslib.as_array(
            (ctypes.c_uint8 * stream.n_elements()).from_address(ptr)
        )
        arr = arr.reshape(*stream.shape()).transpose(1, 2, 0)
        return np.ascontiguousarray(arr)

    def save_image(self: Self, path: PathLike) -> Self:
        path = Path(path).resolve()
        if not path.parent.exists():
            raise FileNotFoundError(
                f'Unable to locate output directory: {path.as_posix()}')
        
        output = self.get_data()
        Image.fromarray(output).save(path)
        return self

