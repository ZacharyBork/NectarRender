"""
Material module.
"""
from __future__ import annotations
import _pathtracer.core.vector
import typing
__all__: list[str] = ['Dielectric', 'Lambertian', 'Material', 'Metal']
class Dielectric(Material):
    def __init__(self, ior: typing.SupportsFloat | typing.SupportsIndex = 1.5) -> None:
        ...
class Lambertian(Material):
    def __init__(self, albedo: _pathtracer.core.vector.Color = ...) -> None:
        ...
class Material:
    pass
class Metal(Material):
    def __init__(self, albedo: _pathtracer.core.vector.Color = ..., fuzz: typing.SupportsFloat | typing.SupportsIndex = 0.0) -> None:
        ...
