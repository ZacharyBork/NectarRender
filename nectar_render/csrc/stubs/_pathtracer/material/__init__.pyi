"""
Material module.
"""
from __future__ import annotations
import _pathtracer.core.vector
import typing
from . import texture
__all__: list[str] = ['Dielectric', 'Emissive', 'Isotropic', 'Lambertian', 'Material', 'Metal', 'texture']
class Dielectric(Material):
    def __init__(self, ior: typing.SupportsFloat | typing.SupportsIndex = 1.5) -> None:
        ...
class Emissive(Material):
    @typing.overload
    def __init__(self, albedo: _pathtracer.core.vector.Color = ...) -> None:
        ...
    @typing.overload
    def __init__(self, arg0: texture.Texture) -> None:
        ...
class Isotropic(Material):
    @typing.overload
    def __init__(self, albedo: _pathtracer.core.vector.Color = ...) -> None:
        ...
    @typing.overload
    def __init__(self, arg0: texture.Texture) -> None:
        ...
class Lambertian(Material):
    @typing.overload
    def __init__(self, albedo: _pathtracer.core.vector.Color = ...) -> None:
        ...
    @typing.overload
    def __init__(self, arg0: texture.Texture) -> None:
        ...
class Material:
    pass
class Metal(Material):
    def __init__(self, albedo: _pathtracer.core.vector.Color = ..., fuzz: typing.SupportsFloat | typing.SupportsIndex = 0.0) -> None:
        ...
