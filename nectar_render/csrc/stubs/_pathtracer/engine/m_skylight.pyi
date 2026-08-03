"""
Skylight submodule.
"""
from __future__ import annotations
import _pathtracer.core.vector
import typing
__all__: list[str] = ['HDRI', 'Simple', 'Skylight', 'SkylightType']
class HDRI:
    pass
class Simple:
    pass
class Skylight:
    @staticmethod
    def hdri(arg0: str, arg1: typing.SupportsFloat | typing.SupportsIndex) -> Skylight:
        ...
    @staticmethod
    def simple(arg0: _pathtracer.core.vector.Color, arg1: _pathtracer.core.vector.Color) -> Skylight:
        ...
    def __init__(self) -> None:
        ...
class SkylightType:
    """
    Members:
    
      Null
    
      Simple
    
      HDRI
    """
    HDRI: typing.ClassVar[SkylightType]  # value = <SkylightType.HDRI: 2>
    Null: typing.ClassVar[SkylightType]  # value = <SkylightType.Null: 0>
    Simple: typing.ClassVar[SkylightType]  # value = <SkylightType.Simple: 1>
    __members__: typing.ClassVar[dict[str, SkylightType]]  # value = {'Null': <SkylightType.Null: 0>, 'Simple': <SkylightType.Simple: 1>, 'HDRI': <SkylightType.HDRI: 2>}
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
