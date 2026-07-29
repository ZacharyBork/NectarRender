"""
Lights submodule.
"""
from __future__ import annotations
import _pathtracer.core.vector
import typing
__all__: list[str] = ['SkyLight']
class SkyLight:
    @staticmethod
    def black() -> SkyLight:
        ...
    @typing.overload
    def __init__(self) -> None:
        ...
    @typing.overload
    def __init__(self, start_color: _pathtracer.core.vector.Color = ..., end_color: _pathtracer.core.vector.Color = ...) -> None:
        ...
