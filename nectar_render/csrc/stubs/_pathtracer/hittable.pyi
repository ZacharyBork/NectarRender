"""
Hittable module.
"""
from __future__ import annotations
import _pathtracer.core.vector
import _pathtracer.material
import typing
__all__: list[str] = ['Hittable', 'Sphere']
class Hittable:
    pass
class Sphere(Hittable):
    def __init__(self, center: _pathtracer.core.vector.Vector3 = ..., radius: typing.SupportsFloat | typing.SupportsIndex = 1.0, material: _pathtracer.material.Material = ...) -> None:
        ...
    def set_motion_vector(self, arg0: _pathtracer.core.vector.Vector3) -> None:
        ...
