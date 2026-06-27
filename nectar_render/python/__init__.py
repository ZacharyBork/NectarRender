import _pathtracer as _C
import atexit; atexit.register(_C.core.utils.destroy_cublas_handle)

from . import (
    camera,
    core,
    data,
    engine,
    hittable,
    material
)

from .engine   import RenderEngine, Transform
from .camera   import Camera
from .core     import Vector2, Vector3, Color, Matrix3
from .material import Material, Texture
from .hittable import Hittable

