"""
Hittable module.
"""
from __future__ import annotations
import _pathtracer.core.vector
import _pathtracer.material
import typing
__all__: list[str] = ['Cube', 'Hittable', 'Quad', 'Sphere']
class Cube(Hittable):
    def __init__(self, position: _pathtracer.core.vector.Vector3 = ..., rotation: _pathtracer.core.vector.Vector3 = ..., scale: _pathtracer.core.vector.Vector3 = ..., material: _pathtracer.material.Material = ...) -> None:
        ...
    def set_motion_vector(self, arg0: _pathtracer.core.vector.Vector3) -> None:
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
