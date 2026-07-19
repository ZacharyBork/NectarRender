"""
Material module.
"""
from __future__ import annotations
import _pathtracer.core.vector
import typing
from . import texture
__all__: list[str] = ['Dielectric', 'Emissive', 'Isotropic', 'Lambertian', 'Material', 'MaterialCore', 'MaterialType', 'PBR', 'texture']
class Dielectric:
    pass
class Emissive:
    pass
class Isotropic:
    pass
class Lambertian:
    pass
class Material:
    @staticmethod
    def dielectric(ior: typing.SupportsFloat | typing.SupportsIndex = 1.5) -> Material:
        ...
    @staticmethod
    @typing.overload
    def emissive(albedo: _pathtracer.core.vector.Color = ..., brightness: typing.SupportsFloat | typing.SupportsIndex = 35.0) -> Material:
        ...
    @staticmethod
    @typing.overload
    def emissive(texture: texture.Texture = ..., brightness: typing.SupportsFloat | typing.SupportsIndex = 35.0) -> Material:
        ...
    @staticmethod
    @typing.overload
    def isotropic(albedo: _pathtracer.core.vector.Color = ...) -> Material:
        ...
    @staticmethod
    @typing.overload
    def isotropic(texture: texture.Texture = ...) -> Material:
        ...
    @staticmethod
    @typing.overload
    def lambertian(albedo: _pathtracer.core.vector.Color = ...) -> Material:
        ...
    @staticmethod
    @typing.overload
    def lambertian(texture: texture.Texture = ...) -> Material:
        ...
    @staticmethod
    def pbr(albedo: texture.Texture = ..., roughness: texture.Texture = ..., metallic: texture.Texture = ..., emission: texture.Texture = ..., normal: texture.Texture = ...) -> Material:
        ...
    def __init__(self) -> None:
        ...
    def __repr__(self) -> str:
        ...
class MaterialCore:
    pass
class MaterialType:
    """
    Members:
    
      Lambertian
    
      PBR
    
      Dielectric
    
      Emissive
    
      Isotropic
    """
    Dielectric: typing.ClassVar[MaterialType]  # value = <MaterialType.Dielectric: 2>
    Emissive: typing.ClassVar[MaterialType]  # value = <MaterialType.Emissive: 3>
    Isotropic: typing.ClassVar[MaterialType]  # value = <MaterialType.Isotropic: 4>
    Lambertian: typing.ClassVar[MaterialType]  # value = <MaterialType.Lambertian: 0>
    PBR: typing.ClassVar[MaterialType]  # value = <MaterialType.PBR: 1>
    __members__: typing.ClassVar[dict[str, MaterialType]]  # value = {'Lambertian': <MaterialType.Lambertian: 0>, 'PBR': <MaterialType.PBR: 1>, 'Dielectric': <MaterialType.Dielectric: 2>, 'Emissive': <MaterialType.Emissive: 3>, 'Isotropic': <MaterialType.Isotropic: 4>}
    def __eq__(self, other: typing.Any) -> bool:
        ...
    def __getstate__(self) -> int:
        ...
    def __hash__(self) -> int:
        ...
    def __index__(self) -> int:
        ...
    def __init__(self, value: typing.SupportsInt | typing.SupportsIndex) -> None:
        ...
    def __int__(self) -> int:
        ...
    def __ne__(self, other: typing.Any) -> bool:
        ...
    @typing.overload
    def __repr__(self) -> str:
        ...
    @typing.overload
    def __repr__(self) -> str:
        ...
    def __setstate__(self, state: typing.SupportsInt | typing.SupportsIndex) -> None:
        ...
    def __str__(self) -> str:
        ...
    @property
    def name(self) -> str:
        ...
    @property
    def value(self) -> int:
        ...
class PBR:
    pass
