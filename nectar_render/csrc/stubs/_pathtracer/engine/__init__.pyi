"""
Engine module.
"""
from __future__ import annotations
import _pathtracer.interface
import collections.abc
import typing
from . import camera
from . import data
from . import denoise
from . import lights
__all__: list[str] = ['EngineState', 'LayerType', 'RenderEngine', 'SampleMode', 'Scene', 'camera', 'data', 'denoise', 'lights']
class EngineState:
    """
    Members:
    
      IDLE
    
      RENDERING
    """
    IDLE: typing.ClassVar[EngineState]  # value = <EngineState.IDLE: 0>
    RENDERING: typing.ClassVar[EngineState]  # value = <EngineState.RENDERING: 1>
    __members__: typing.ClassVar[dict[str, EngineState]]  # value = {'IDLE': <EngineState.IDLE: 0>, 'RENDERING': <EngineState.RENDERING: 1>}
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
    on_frame_finished: collections.abc.Callable[[typing.SupportsInt | typing.SupportsIndex], None]
    on_render_finished: collections.abc.Callable[[], None]
    on_render_started: collections.abc.Callable[[], None]
    on_reset: collections.abc.Callable[[], None]
    on_stopped: collections.abc.Callable[[], None]
    def __init__(self, camera: camera.Camera, ray_depth: typing.SupportsInt | typing.SupportsIndex = 8, seed: typing.SupportsInt | typing.SupportsIndex = 54321) -> None:
        ...
    def camera(self) -> camera.Camera:
        ...
    def get_state(self) -> EngineState:
        ...
    def is_idle(self) -> bool:
        ...
    def is_rendering(self) -> bool:
        ...
    def layers(self) -> data.RenderLayers:
        ...
    def max_depth(self) -> int:
        ...
    def n_samples(self) -> int:
        ...
    def queue_function(self, func: collections.abc.Callable[[], None], rebuild_bvh: bool = False, immediate: bool = True) -> None:
        ...
    def render(self, mode: SampleMode = ...) -> None:
        ...
    def request_stop(self) -> None:
        ...
    def reset(self) -> None:
        ...
    def restart(self) -> None:
        ...
    def sample(self, sample_idx: typing.SupportsInt | typing.SupportsIndex = 0, mode: SampleMode = ...) -> None:
        ...
    def scene(self) -> Scene:
        ...
    def screen_space_ray(self, arg0: typing.SupportsFloat | typing.SupportsIndex, arg1: typing.SupportsFloat | typing.SupportsIndex) -> _pathtracer.interface.ObjectInterface:
        ...
    def set_max_depth(self, arg0: typing.SupportsInt | typing.SupportsIndex) -> None:
        ...
    def set_n_samples(self, arg0: typing.SupportsInt | typing.SupportsIndex) -> None:
        ...
    def set_scene(self, scene: Scene) -> None:
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
