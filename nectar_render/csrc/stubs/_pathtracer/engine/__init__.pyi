"""
Engine module.
"""
from __future__ import annotations
import _pathtracer.core.matrix
import _pathtracer.core.vector
import collections.abc
import typing
from . import camera
from . import data
from . import denoise
from . import lights
__all__: list[str] = ['LayerType', 'RenderEngine', 'SampleMode', 'Scene', 'Transform', 'camera', 'data', 'denoise', 'lights']
class LayerType:
    """
    Members:
    
      BEAUTY
    
      DIFFUSE
    
      SPECULAR
    
      NORMAL
    
      SHADOW
    
      DEPTH
    
      EMISSION
    
      OBJECT_ID
    """
    BEAUTY: typing.ClassVar[LayerType]  # value = <LayerType.BEAUTY: 0>
    DEPTH: typing.ClassVar[LayerType]  # value = <LayerType.DEPTH: 5>
    DIFFUSE: typing.ClassVar[LayerType]  # value = <LayerType.DIFFUSE: 1>
    EMISSION: typing.ClassVar[LayerType]  # value = <LayerType.EMISSION: 6>
    NORMAL: typing.ClassVar[LayerType]  # value = <LayerType.NORMAL: 3>
    OBJECT_ID: typing.ClassVar[LayerType]  # value = <LayerType.OBJECT_ID: 7>
    SHADOW: typing.ClassVar[LayerType]  # value = <LayerType.SHADOW: 4>
    SPECULAR: typing.ClassVar[LayerType]  # value = <LayerType.SPECULAR: 2>
    __members__: typing.ClassVar[dict[str, LayerType]]  # value = {'BEAUTY': <LayerType.BEAUTY: 0>, 'DIFFUSE': <LayerType.DIFFUSE: 1>, 'SPECULAR': <LayerType.SPECULAR: 2>, 'NORMAL': <LayerType.NORMAL: 3>, 'SHADOW': <LayerType.SHADOW: 4>, 'DEPTH': <LayerType.DEPTH: 5>, 'EMISSION': <LayerType.EMISSION: 6>, 'OBJECT_ID': <LayerType.OBJECT_ID: 7>}
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
class RenderEngine:
    on_frame_finished: collections.abc.Callable[[typing.SupportsInt | typing.SupportsIndex], None]
    def __init__(self, camera: camera.Camera, ray_depth: typing.SupportsInt | typing.SupportsIndex = 8, seed: typing.SupportsInt | typing.SupportsIndex = 54321) -> None:
        ...
    def layers(self) -> data.RenderLayers:
        ...
    def render(self, scene: Scene, mode: SampleMode = ...) -> None:
        ...
    def sample(self, scene: Scene, mode: SampleMode = ...) -> None:
        ...
class SampleMode:
    """
    Members:
    
      ACCUMULATE
    
      COMBINE
    """
    ACCUMULATE: typing.ClassVar[SampleMode]  # value = <SampleMode.ACCUMULATE: 0>
    COMBINE: typing.ClassVar[SampleMode]  # value = <SampleMode.COMBINE: 1>
    __members__: typing.ClassVar[dict[str, SampleMode]]  # value = {'ACCUMULATE': <SampleMode.ACCUMULATE: 0>, 'COMBINE': <SampleMode.COMBINE: 1>}
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
class Scene:
    def __init__(self, hittables: list, skylight: lights.SkyLight) -> None:
        ...
class Transform:
    def R(self) -> _pathtracer.core.matrix.Matrix3:
        ...
    @typing.overload
    def __init__(self) -> None:
        ...
    @typing.overload
    def __init__(self, position: _pathtracer.core.vector.Vector3 = ..., rotation: _pathtracer.core.vector.Vector3 = ..., scale: _pathtracer.core.vector.Vector3 = ...) -> None:
        ...
    @typing.overload
    def __init__(self, position: typing.Annotated[collections.abc.Sequence[typing.SupportsFloat | typing.SupportsIndex], "FixedSize(3)"] = [0.0, 0.0, 0.0], rotation: typing.Annotated[collections.abc.Sequence[typing.SupportsFloat | typing.SupportsIndex], "FixedSize(3)"] = [0.0, 0.0, 0.0], scale: typing.Annotated[collections.abc.Sequence[typing.SupportsFloat | typing.SupportsIndex], "FixedSize(3)"] = [1.0, 1.0, 1.0]) -> None:
        ...
    def p(self) -> _pathtracer.core.vector.Vector3:
        ...
    def pos(self) -> _pathtracer.core.vector.Vector3:
        ...
    def position(self) -> _pathtracer.core.vector.Vector3:
        ...
    def rotation(self) -> _pathtracer.core.matrix.Matrix3:
        ...
    def scale(self) -> _pathtracer.core.vector.Vector3:
        ...
    def set_position(self, arg0: _pathtracer.core.vector.Vector3) -> None:
        ...
    def set_rotation(self, arg0: _pathtracer.core.matrix.Matrix3) -> None:
        ...
    def set_scale(self, arg0: _pathtracer.core.vector.Vector3) -> None:
        ...
