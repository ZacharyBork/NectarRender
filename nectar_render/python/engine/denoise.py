import _pathtracer
root = _pathtracer.engine.denoise

from typing import Self, TypeAlias

from nectar_render.python.engine.data import DataObject

Denoiser: TypeAlias = root.Denoiser

class TVDenoiser(root.TVDenoiser):
    def __init__(
        self:       Self,
        weight:     float = 0.05,
        iterations: int = 100
    ) -> None:
        super().__init__(weight, iterations)
        
    def run(self: Self, data: DataObject) -> None:
        super().run(data)
        

