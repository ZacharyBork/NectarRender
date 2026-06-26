"""
Texture submodule.
"""
from __future__ import annotations
import _pathtracer.core.vector
import typing
__all__: list[str] = ['CheckerTexture', 'ConstantTexture', 'Texture']
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
class Texture:
    pass
