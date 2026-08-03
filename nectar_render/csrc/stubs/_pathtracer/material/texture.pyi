"""
Texture submodule.
"""
from __future__ import annotations
import _pathtracer.core.vector
import typing
__all__: list[str] = ['Texture', 'TextureType']
class Texture:
    @staticmethod
    @typing.overload
    def from_color(color: _pathtracer.core.vector.Color) -> Texture:
        ...
    @staticmethod
    @typing.overload
    def from_color(r: typing.SupportsFloat | typing.SupportsIndex, g: typing.SupportsFloat | typing.SupportsIndex, b: typing.SupportsFloat | typing.SupportsIndex) -> Texture:
        ...
    @staticmethod
    def from_image(filepath: str) -> Texture:
        ...
    def __repr__(self) -> str:
        ...
    @property
    def C(self) -> int:
        ...
    @property
    def H(self) -> int:
        ...
    @property
    def W(self) -> int:
        ...
    @property
    def constant_color(self) -> _pathtracer.core.vector.Color:
        ...
    @property
    def filepath(self) -> str:
        ...
    @property
    def type(self) -> TextureType:
        ...
class TextureType:
    """
    Members:
    
      CONSTANT
    
      IMAGE
    """
    CONSTANT: typing.ClassVar[TextureType]  # value = <TextureType.CONSTANT: 0>
    IMAGE: typing.ClassVar[TextureType]  # value = <TextureType.IMAGE: 1>
    __members__: typing.ClassVar[dict[str, TextureType]]  # value = {'CONSTANT': <TextureType.CONSTANT: 0>, 'IMAGE': <TextureType.IMAGE: 1>}
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
