"""
Data module.
"""
from __future__ import annotations
import typing
__all__: list[str] = ['DataObject', 'DenoiseFilterQuality', 'LayerType', 'RenderLayers', 'RenderLayersConfig', 'StreamConfig', 'StreamState', 'TonemapMethod', 'TransferStream']
class DataObject:
    def device_ptr(self) -> int:
        ...
    def is_enabled(self) -> bool:
        ...
    def n_bytes(self) -> int:
        ...
    def n_elements(self) -> int:
        ...
    def n_pixels(self) -> int:
        ...
    def numpy(self) -> numpy.ndarray:
        ...
    def shape(self) -> list[int]:
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
class DenoiseFilterQuality:
    """
    Members:
    
      DEFAULT
    
      HIGH
    
      BALANCED
    
      FAST
    """
    BALANCED: typing.ClassVar[DenoiseFilterQuality]  # value = <DenoiseFilterQuality.BALANCED: 2>
    DEFAULT: typing.ClassVar[DenoiseFilterQuality]  # value = <DenoiseFilterQuality.DEFAULT: 0>
    FAST: typing.ClassVar[DenoiseFilterQuality]  # value = <DenoiseFilterQuality.FAST: 3>
    HIGH: typing.ClassVar[DenoiseFilterQuality]  # value = <DenoiseFilterQuality.HIGH: 1>
    __members__: typing.ClassVar[dict[str, DenoiseFilterQuality]]  # value = {'DEFAULT': <DenoiseFilterQuality.DEFAULT: 0>, 'HIGH': <DenoiseFilterQuality.HIGH: 1>, 'BALANCED': <DenoiseFilterQuality.BALANCED: 2>, 'FAST': <DenoiseFilterQuality.FAST: 3>}
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
class LayerType:
    """
    Members:
    
      BEAUTY
    
      DIFFUSE
    
      WORLD_NORMAL
    
      OBJECT_NORMAL
    
      SPECULAR
    
      SHADOW
    
      DEPTH
    
      EMISSION
    
      OBJECT_ID
    """
    BEAUTY: typing.ClassVar[LayerType]  # value = <LayerType.BEAUTY: 0>
    DEPTH: typing.ClassVar[LayerType]  # value = <LayerType.DEPTH: 6>
    DIFFUSE: typing.ClassVar[LayerType]  # value = <LayerType.DIFFUSE: 1>
    EMISSION: typing.ClassVar[LayerType]  # value = <LayerType.EMISSION: 7>
    OBJECT_ID: typing.ClassVar[LayerType]  # value = <LayerType.OBJECT_ID: 8>
    OBJECT_NORMAL: typing.ClassVar[LayerType]  # value = <LayerType.OBJECT_NORMAL: 3>
    SHADOW: typing.ClassVar[LayerType]  # value = <LayerType.SHADOW: 5>
    SPECULAR: typing.ClassVar[LayerType]  # value = <LayerType.SPECULAR: 2>
    WORLD_NORMAL: typing.ClassVar[LayerType]  # value = <LayerType.WORLD_NORMAL: 4>
    __members__: typing.ClassVar[dict[str, LayerType]]  # value = {'BEAUTY': <LayerType.BEAUTY: 0>, 'DIFFUSE': <LayerType.DIFFUSE: 1>, 'WORLD_NORMAL': <LayerType.WORLD_NORMAL: 4>, 'OBJECT_NORMAL': <LayerType.OBJECT_NORMAL: 3>, 'SPECULAR': <LayerType.SPECULAR: 2>, 'SHADOW': <LayerType.SHADOW: 5>, 'DEPTH': <LayerType.DEPTH: 6>, 'EMISSION': <LayerType.EMISSION: 7>, 'OBJECT_ID': <LayerType.OBJECT_ID: 8>}
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
class RenderLayers:
    def __init__(self, h: typing.SupportsInt | typing.SupportsIndex = True, w: typing.SupportsInt | typing.SupportsIndex = False, cfg: RenderLayersConfig = False) -> None:
        ...
    def get_layer(self, arg0: LayerType) -> DataObject:
        ...
    def normalize_by_samples(self, arg0: typing.SupportsInt | typing.SupportsIndex) -> None:
        ...
    @property
    def H(self) -> int:
        ...
    @property
    def W(self) -> int:
        ...
    @property
    def beauty(self) -> DataObject:
        ...
    @property
    def depth(self) -> DataObject:
        ...
    @property
    def diffuse(self) -> DataObject:
        ...
    @property
    def emission(self) -> DataObject:
        ...
    @property
    def object_id(self) -> DataObject:
        ...
    @property
    def object_normal(self) -> DataObject:
        ...
    @property
    def shadow(self) -> DataObject:
        ...
    @property
    def specular(self) -> DataObject:
        ...
    @property
    def world_normal(self) -> DataObject:
        ...
class RenderLayersConfig:
    beauty: bool
    depth: bool
    diffuse: bool
    emission: bool
    object_id: bool
    object_normal: bool
    shadow: bool
    specular: bool
    world_normal: bool
    @typing.overload
    def __init__(self) -> None:
        ...
    @typing.overload
    def __init__(self, beauty: bool = True, diffuse: bool = True, world_normal: bool = True, object_normal: bool = False, specular: bool = False, shadow: bool = False, depth: bool = False, emission: bool = False, object_id: bool = False) -> None:
        ...
class StreamConfig:
    apply_denoising: bool
    apply_tonemapping: bool
    apply_white_balance: bool
    denoise_clean_auxiliaries: bool
    denoise_quality: DenoiseFilterQuality
    linear_to_gamma: bool
    tm_method: TonemapMethod
    def __init__(self) -> None:
        ...
    @property
    def denoise_input_scale(self) -> float:
        ...
    @denoise_input_scale.setter
    def denoise_input_scale(self, arg0: typing.SupportsFloat | typing.SupportsIndex) -> None:
        ...
    @property
    def tm_alpha(self) -> float:
        ...
    @tm_alpha.setter
    def tm_alpha(self, arg0: typing.SupportsFloat | typing.SupportsIndex) -> None:
        ...
    @property
    def tm_white_point(self) -> float:
        ...
    @tm_white_point.setter
    def tm_white_point(self, arg0: typing.SupportsFloat | typing.SupportsIndex) -> None:
        ...
    @property
    def wb_temperature(self) -> float:
        ...
    @wb_temperature.setter
    def wb_temperature(self, arg0: typing.SupportsFloat | typing.SupportsIndex) -> None:
        ...
    @property
    def wb_tint(self) -> float:
        ...
    @wb_tint.setter
    def wb_tint(self, arg0: typing.SupportsFloat | typing.SupportsIndex) -> None:
        ...
class StreamState:
    """
    Members:
    
      ACTIVE
    
      INACTIVE
    
      FROZEN
    """
    ACTIVE: typing.ClassVar[StreamState]  # value = <StreamState.ACTIVE: 0>
    FROZEN: typing.ClassVar[StreamState]  # value = <StreamState.INACTIVE: 1>
    INACTIVE: typing.ClassVar[StreamState]  # value = <StreamState.INACTIVE: 1>
    __members__: typing.ClassVar[dict[str, StreamState]]  # value = {'ACTIVE': <StreamState.ACTIVE: 0>, 'INACTIVE': <StreamState.INACTIVE: 1>, 'FROZEN': <StreamState.INACTIVE: 1>}
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
class TonemapMethod:
    """
    Members:
    
      REINHARD
    
      REINHARD_EXTENDED
    
      ACES
    """
    ACES: typing.ClassVar[TonemapMethod]  # value = <TonemapMethod.ACES: 2>
    REINHARD: typing.ClassVar[TonemapMethod]  # value = <TonemapMethod.REINHARD: 0>
    REINHARD_EXTENDED: typing.ClassVar[TonemapMethod]  # value = <TonemapMethod.REINHARD_EXTENDED: 1>
    __members__: typing.ClassVar[dict[str, TonemapMethod]]  # value = {'REINHARD': <TonemapMethod.REINHARD: 0>, 'REINHARD_EXTENDED': <TonemapMethod.REINHARD_EXTENDED: 1>, 'ACES': <TonemapMethod.ACES: 2>}
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
class TransferStream:
    def buffer_ptr(self) -> int:
        ...
    def disable_denoising(self) -> None:
        ...
    def enable_denoising(self) -> None:
        ...
    def get_state(self) -> StreamState:
        ...
    def has_overlay(self) -> bool:
        ...
    def is_active(self) -> bool:
        ...
    def is_inactive(self) -> bool:
        ...
    def is_linked(self) -> bool:
        ...
    def n_bytes(self) -> int:
        ...
    def n_elements(self) -> int:
        ...
    def n_pixels(self) -> int:
        ...
    @typing.overload
    def readback(self) -> int:
        ...
    @typing.overload
    def readback(self) -> int:
        ...
    def rebuild_denoiser(self) -> None:
        ...
    def remove_overlay(self) -> None:
        ...
    def shape(self) -> typing.Annotated[list[int], "FixedSize(3)"]:
        ...
    def update_config(self, arg0: StreamConfig) -> None:
        ...
