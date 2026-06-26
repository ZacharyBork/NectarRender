"""
Texture submodule.
"""
from __future__ import annotations
import _pathtracer.core.vector
import typing
__all__: list[str] = ['CheckerTexture', 'ConstantTexture', 'ImageTexture', 'NoiseTexture', 'Texture']
class CheckerTexture(Texture):
    def __init__(self, color1: _pathtracer.core.vector.Color = ..., color1: _pathtracer.core.vector.Color = ..., scale: typing.SupportsFloat | typing.SupportsIndex = 1.0) -> None:
        ...
class ConstantTexture(Texture):
    @typing.overload
    def __init__(self, albedo: _pathtracer.core.vector.Color = ...) -> None:
        ...
    @typing.overload
    def __init__(self, r: typing.SupportsFloat | typing.SupportsIndex = 0.800000011920929, g: typing.SupportsFloat | typing.SupportsIndex = 0.800000011920929, b: typing.SupportsFloat | typing.SupportsIndex = 0.800000011920929) -> None:
        ...
class ImageTexture(Texture):
    def __init__(self, host_ptr: typing.SupportsInt | typing.SupportsIndex, channels: typing.SupportsInt | typing.SupportsIndex, height: typing.SupportsInt | typing.SupportsIndex, width: typing.SupportsInt | typing.SupportsIndex) -> None:
        ...
    def n_bytes(self) -> int:
        ...
class NoiseTexture(Texture):
    @typing.overload
    def __init__(self) -> None:
        ...
    @typing.overload
    def __init__(self, seed: typing.SupportsInt | typing.SupportsIndex) -> None:
        ...
class Texture:
    pass
