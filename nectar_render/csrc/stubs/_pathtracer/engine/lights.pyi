"""
Lights submodule.
"""
from __future__ import annotations
import _pathtracer.core.vector
import _pathtracer.hittable
import _pathtracer.material.texture
import typing
__all__: list[str] = ['Light', 'ObjectLight', 'SkyLight']
class Light(_pathtracer.hittable.Hittable):
    pass
class ObjectLight(Light):
    @typing.overload
    def __init__(self, obj: _pathtracer.hittable.Hittable, brightness: typing.SupportsFloat | typing.SupportsIndex = 35.0, albedo: _pathtracer.core.vector.Color = ...) -> None:
        ...
    @typing.overload
    def __init__(self, obj: _pathtracer.hittable.Hittable, brightness: typing.SupportsFloat | typing.SupportsIndex = 35.0, texture: _pathtracer.material.texture.Texture = ...) -> None:
        ...
class SkyLight:
    @staticmethod
    def black() -> SkyLight:
        ...
    @typing.overload
    def __init__(self) -> None:
        ...
    @typing.overload
    def __init__(self, start_color: _pathtracer.core.vector.Color = ..., end_color: _pathtracer.core.vector.Color = ...) -> None:
        ...
