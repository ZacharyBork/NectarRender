"""
Denoising submodule.
"""
from __future__ import annotations
import _pathtracer.data
import typing
__all__: list[str] = ['Denoiser', 'TVDenoiser']
class Denoiser:
    def run(self, arg0: _pathtracer.data.DataObject) -> None:
        ...
class TVDenoiser(Denoiser):
    def __init__(self, weight: typing.SupportsFloat | typing.SupportsIndex = 1.0, iterations: typing.SupportsInt | typing.SupportsIndex = 100) -> None:
        ...
    def run(self, arg0: _pathtracer.data.DataObject) -> None:
        ...
