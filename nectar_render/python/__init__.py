import _pathtracer as _C
import atexit; atexit.register(_C.core.utils.destroy_cublas_handle)

from .          import core, hittable, material, engine
from .core      import Vector2, Vector3, Color, Matrix3
from .host      import CUDAMemInfo, CUDAProfiler
from .material  import Material, Texture
from .hittable  import Hittable, Volumetric, HitRecord
from .interface import ObjectInterface
from .engine    import *


