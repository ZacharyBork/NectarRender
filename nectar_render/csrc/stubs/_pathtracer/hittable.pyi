"""
Hittable module.
"""
from __future__ import annotations
import _pathtracer.core.vector
import typing
__all__: list[str] = ['Hittable', 'Sphere']
class Hittable:
    pass
class Sphere:
    def __init__(self, center: _pathtracer.core.vector.Vector3 = ..., radius: typing.SupportsFloat | typing.SupportsIndex = 1.0, material: Material = ...) -> None:
        ...
