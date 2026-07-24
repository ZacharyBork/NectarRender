"""
Texture submodule.
"""
from __future__ import annotations
import _pathtracer.core.vector
import typing
__all__: list[str] = ['Texture']
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
    def from_image(filepath: str, host_ptr: typing.SupportsInt | typing.SupportsIndex, channels: typing.SupportsInt | typing.SupportsIndex, height: typing.SupportsInt | typing.SupportsIndex, width: typing.SupportsInt | typing.SupportsIndex) -> Texture:
        ...
