"""
Hittable module.
"""
from __future__ import annotations
import _pathtracer.core.vector
import _pathtracer.material
import _pathtracer.material.texture
import typing
__all__: list[str] = ['ConstantMedium', 'Cube', 'HitRecord', 'Hittable', 'Quad', 'Sphere']
class ConstantMedium(Hittable):
    @typing.overload
    def __init__(self, boundary: Hittable, density: typing.SupportsFloat | typing.SupportsIndex = 1.0, albedo: _pathtracer.core.vector.Color = ...) -> None:
        ...
    @typing.overload
    def __init__(self, boundary: Hittable, density: typing.SupportsFloat | typing.SupportsIndex = 1.0, texture: _pathtracer.material.texture.Texture = ...) -> None:
        ...
    def set_motion_vector(self, arg0: _pathtracer.core.vector.Vector3) -> None:
        ...
class Cube(Hittable):
    def __init__(self, position: _pathtracer.core.vector.Vector3 = ..., rotation: _pathtracer.core.vector.Vector3 = ..., scale: _pathtracer.core.vector.Vector3 = ..., material: _pathtracer.material.Material = ...) -> None:
        ...
    def set_motion_vector(self, arg0: _pathtracer.core.vector.Vector3) -> None:
        ...
class HitRecord:
    def __init__(self) -> None:
        ...
    def d_object_ptr(self) -> int:
        ...
    @property
    def front_face(self) -> bool:
        ...
    @property
    def hit_object(self) -> Hittable:
        ...
    @property
    def mat(self) -> _pathtracer.material.Material:
        ...
    @property
    def n(self) -> _pathtracer.core.vector.Vector3:
        ...
    @property
    def object_index(self) -> int:
        ...
    @property
    def p(self) -> _pathtracer.core.vector.Vector3:
        ...
    @property
    def t(self) -> float:
        ...
    @property
    def tangent(self) -> _pathtracer.core.vector.Vector3:
        ...
    @property
    def uv(self) -> _pathtracer.core.vector.Vector2:
        ...
class Hittable:
    pass
class Quad(Hittable):
    def __init__(self, position: _pathtracer.core.vector.Vector3 = ..., rotation: _pathtracer.core.vector.Vector3 = ..., scale: _pathtracer.core.vector.Vector3 = ..., material: _pathtracer.material.Material = ...) -> None:
        ...
    def set_motion_vector(self, arg0: _pathtracer.core.vector.Vector3) -> None:
        ...
class Sphere(Hittable):
    def __init__(self, center: _pathtracer.core.vector.Vector3 = ..., radius: typing.SupportsFloat | typing.SupportsIndex = 1.0, material: _pathtracer.material.Material = ...) -> None:
        ...
    def set_motion_vector(self, arg0: _pathtracer.core.vector.Vector3) -> None:
        ...
