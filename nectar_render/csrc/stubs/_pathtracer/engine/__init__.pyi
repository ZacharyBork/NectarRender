"""
Engine module.
"""
from __future__ import annotations
import _pathtracer.core.matrix
import _pathtracer.core.vector
import _pathtracer.hittable
import collections.abc
import typing
from . import camera
from . import data
from . import denoise
from . import lights
__all__: list[str] = ['EngineState', 'LayerType', 'RenderEngine', 'SampleMode', 'Scene', 'Transform', 'camera', 'data', 'denoise', 'lights']
class EngineState:
    """
    Members:
    
      IDLE
    
      RENDERING
    
      PAUSED
    
      CANCELLED
    
      RESETTING
    """
    CANCELLED: typing.ClassVar[EngineState]  # value = <EngineState.CANCELLED: 3>
    IDLE: typing.ClassVar[EngineState]  # value = <EngineState.IDLE: 0>
    PAUSED: typing.ClassVar[EngineState]  # value = <EngineState.PAUSED: 2>
    RENDERING: typing.ClassVar[EngineState]  # value = <EngineState.RENDERING: 1>
    RESETTING: typing.ClassVar[EngineState]  # value = <EngineState.RESETTING: 4>
    __members__: typing.ClassVar[dict[str, EngineState]]  # value = {'IDLE': <EngineState.IDLE: 0>, 'RENDERING': <EngineState.RENDERING: 1>, 'PAUSED': <EngineState.PAUSED: 2>, 'CANCELLED': <EngineState.CANCELLED: 3>, 'RESETTING': <EngineState.RESETTING: 4>}
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
    on_canceled: collections.abc.Callable[[], None]
    on_frame_finished: collections.abc.Callable[[typing.SupportsInt | typing.SupportsIndex], None]
    on_paused: collections.abc.Callable[[], None]
    on_render_finished: collections.abc.Callable[[], None]
    on_render_started: collections.abc.Callable[[], None]
    on_reset: collections.abc.Callable[[], None]
    def __init__(self, camera: camera.Camera, ray_depth: typing.SupportsInt | typing.SupportsIndex = 8, seed: typing.SupportsInt | typing.SupportsIndex = 54321) -> None:
        ...
    def camera(self) -> camera.Camera:
        ...
    def get_state(self) -> EngineState:
        ...
    def is_cancelled(self) -> bool:
        ...
    def is_idle(self) -> bool:
        ...
    def is_paused(self) -> bool:
        ...
    def is_refreshing(self) -> bool:
        ...
    def is_rendering(self) -> bool:
        ...
    def layers(self) -> data.RenderLayers:
        ...
    def n_samples(self) -> int:
        ...
    def render(self, scene: Scene, mode: SampleMode = ...) -> None:
        ...
    def request_cancel(self) -> None:
        ...
    def request_pause(self) -> None:
        ...
    def request_reset(self) -> None:
        ...
    def reset(self) -> None:
        ...
    def sample(self, scene: Scene, sample_idx: typing.SupportsInt | typing.SupportsIndex = 0, mode: SampleMode = ...) -> None:
        ...
    def scene(self) -> Scene:
        ...
    def screen_space_ray(self, arg0: typing.SupportsFloat | typing.SupportsIndex, arg1: typing.SupportsFloat | typing.SupportsIndex) -> _pathtracer.hittable.HitRecord:
        ...
    def set_n_samples(self, arg0: typing.SupportsInt | typing.SupportsIndex) -> None:
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
    def __init__(self, hittables: list, lights: list, skylight: lights.SkyLight) -> None:
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
