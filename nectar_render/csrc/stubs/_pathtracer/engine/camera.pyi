"""
Camera submodule.
"""
from __future__ import annotations
import _pathtracer.core.matrix
import _pathtracer.core.vector
import collections.abc
import typing
__all__: list[str] = ['Camera', 'CameraParams']
class Camera:
    on_updated: collections.abc.Callable[[CameraParams], None]
    def __init__(self, params: CameraParams) -> None:
        ...
    def parameters(self) -> CameraParams:
        ...
class CameraParams:
    position: _pathtracer.core.vector.Vector3
    resolution: _pathtracer.core.vector.Vector2
    rotation: _pathtracer.core.vector.Vector3
    def __init__(self, resolution: _pathtracer.core.vector.Vector2, position: _pathtracer.core.vector.Vector3, rotation: _pathtracer.core.vector.Vector3, samples_per_pixel: typing.SupportsInt | typing.SupportsIndex, focal_length: typing.SupportsFloat | typing.SupportsIndex, focus_distance: typing.SupportsFloat | typing.SupportsIndex, aperture: typing.SupportsFloat | typing.SupportsIndex, sensor_width: typing.SupportsFloat | typing.SupportsIndex, shutter_speed: typing.SupportsFloat | typing.SupportsIndex) -> None:
        ...
    @property
    def R(self) -> _pathtracer.core.matrix.Matrix3:
        ...
    @property
    def aperture(self) -> float:
        ...
    @aperture.setter
    def aperture(self, arg0: typing.SupportsFloat | typing.SupportsIndex) -> None:
        ...
    @property
    def aspect_ratio(self) -> float:
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
    def samples_per_pixel(self) -> int:
        ...
    @samples_per_pixel.setter
    def samples_per_pixel(self, arg0: typing.SupportsInt | typing.SupportsIndex) -> None:
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
    @property
    def shutter_time(self) -> float:
        ...
