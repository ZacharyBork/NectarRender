import _pathtracer

from typing import Any, Self, TypeAlias

CameraParams: TypeAlias = _pathtracer.engine.CameraParams

class Camera:
    _CDATA: CameraParams = None
    resolution:     tuple[int, int]
    position:       tuple[float, float, float]
    rotation:       tuple[float, float, float]
    focal_length:   float
    focus_distance: float
    aperture:       float
    sensor_width:   float
    shutter_speed:  float
    
    def __init__(
        self:       Self,
        resolution: tuple[int, int] = (512, 512),
        position:   tuple[float, float, float] = (0.0, 0.0, 0.0),
        rotation:   tuple[float, float, float] = (0.0, 0.0, 0.0),
        focal_length:   float = 5.0,
        focus_distance: float = 10.0,
        aperture:       float = 0.01,
        sensor_width:   float = 2.0,
        shutter_speed:  float = 1.0
    ) -> None:
        for key, value in { 
            k: v for k, v in locals().items() if not k == 'self' 
        }.items(): self.__dict__[key] = value
        
        self._build_cdata()
        
    def _build_cdata(self: Self) -> None:
        self._CDATA = CameraParams(*list(self.__dict__.values()))
        
    @property
    def cdata(self: Self) -> CameraParams: return self._CDATA
    
    @property
    def params(self: Self) -> dict[str, Any]:
        return { 
            k: v for k, v in self.__dict__.items() 
            if not k == '_CDATA' 
        }
        

