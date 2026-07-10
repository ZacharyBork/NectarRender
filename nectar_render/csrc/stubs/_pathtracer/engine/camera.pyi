"""
Camera submodule.
"""
from __future__ import annotations
import _pathtracer.core.matrix
import _pathtracer.core.vector
import collections.abc
import typing
__all__: list[str] = ['Camera']
class Camera:
    position: _pathtracer.core.vector.Vector3
    resolution: _pathtracer.core.vector.Vector2
    rotation: _pathtracer.core.matrix.Matrix3
    def __init__(self, resolution: typing.Annotated[collections.abc.Sequence[typing.SupportsInt | typing.SupportsIndex], "FixedSize(2)"] = [512, 512], position: typing.Annotated[collections.abc.Sequence[typing.SupportsFloat | typing.SupportsIndex], "FixedSize(3)"] = [0.0, 0.0, 0.0], rotation: typing.Annotated[collections.abc.Sequence[typing.SupportsFloat | typing.SupportsIndex], "FixedSize(3)"] = [0.0, 0.0, 0.0], samples_per_pixel: typing.SupportsInt | typing.SupportsIndex = 500, focal_length: typing.SupportsFloat | typing.SupportsIndex = 5.0, focus_distance: typing.SupportsFloat | typing.SupportsIndex = 10.0, aperture: typing.SupportsFloat | typing.SupportsIndex = 0.009999999776482582, sensor_width: typing.SupportsFloat | typing.SupportsIndex = 2.0, shutter_speed: typing.SupportsFloat | typing.SupportsIndex = 1.0) -> None:
        ...
    def update(self, arg0: _pathtracer.core.vector.Vector3, arg1: _pathtracer.core.vector.Vector3) -> None:
        ...
    @property
    def aperture(self) -> float:
        ...
    @aperture.setter
    def aperture(self, arg0: typing.SupportsFloat | typing.SupportsIndex) -> None:
        ...
    @property
    def focal_length(self) -> float:
        ...
    @focal_length.setter
    def focal_length(self, arg0: typing.SupportsFloat | typing.SupportsIndex) -> None:
        ...
    @property
    def focus_distance(self) -> float:
        ...
    @focus_distance.setter
    def focus_distance(self, arg0: typing.SupportsFloat | typing.SupportsIndex) -> None:
        ...
    @property
    def n_samples(self) -> int:
        ...
    @n_samples.setter
    def n_samples(self, arg0: typing.SupportsInt | typing.SupportsIndex) -> None:
        ...
    @property
    def sensor_width(self) -> float:
        ...
    @sensor_width.setter
    def sensor_width(self, arg0: typing.SupportsFloat | typing.SupportsIndex) -> None:
        ...
    @property
    def shutter_speed(self) -> float:
        ...
    @shutter_speed.setter
    def shutter_speed(self, arg0: typing.SupportsFloat | typing.SupportsIndex) -> None:
        ...
