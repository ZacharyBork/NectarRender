"""
Hittable module.
"""
from __future__ import annotations
import _pathtracer.core.vector
import _pathtracer.material
import typing
__all__: list[str] = ['ConstantMedium', 'Cube', 'HitRecord', 'Hittable', 'Mesh', 'ObjectLight', 'Quad', 'Sphere']
class ConstantMedium:
    pass
class Cube:
    pass
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
    def object_id(self) -> int:
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
    @staticmethod
    @typing.overload
    def constant_medium(*args, **kwargs) -> Hittable:
        ...
    @staticmethod
    @typing.overload
    def constant_medium(bound_obj: Hittable, density: typing.SupportsFloat | typing.SupportsIndex = 1.0, albedo: _pathtracer.core.vector.Color = ...) -> Hittable:
        ...
    @staticmethod
    def cube(position: _pathtracer.core.vector.Vector3 = ..., rotation: _pathtracer.core.vector.Vector3 = ..., scale: _pathtracer.core.vector.Vector3 = ..., material: _pathtracer.material.Material = ...) -> Hittable:
        ...
    @staticmethod
    def mesh(path: str, position: _pathtracer.core.vector.Vector3 = ..., rotation: _pathtracer.core.vector.Vector3 = ..., scale: _pathtracer.core.vector.Vector3 = ..., material: _pathtracer.material.Material = ...) -> Hittable:
        ...
    @staticmethod
    @typing.overload
    def object_light(*args, **kwargs) -> Hittable:
        ...
    @staticmethod
    @typing.overload
    def object_light(bound_obj: Hittable, brightness: typing.SupportsFloat | typing.SupportsIndex = 1.0, albedo: _pathtracer.core.vector.Color = ...) -> Hittable:
        ...
    @staticmethod
    def quad(position: _pathtracer.core.vector.Vector3 = ..., rotation: _pathtracer.core.vector.Vector3 = ..., scale: _pathtracer.core.vector.Vector3 = ..., material: _pathtracer.material.Material = ...) -> Hittable:
        ...
    @staticmethod
    def sphere(position: _pathtracer.core.vector.Vector3 = ..., radius: typing.SupportsFloat | typing.SupportsIndex = 1.0, material: _pathtracer.material.Material = ...) -> Hittable:
        ...
    def __init__(self) -> None:
        ...
class Mesh:
    pass
class ObjectLight:
    pass
class Quad:
    pass
class Sphere:
    pass
