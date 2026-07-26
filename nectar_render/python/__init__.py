import _pathtracer as _C
import atexit; atexit.register(_C.cuda.destroy_cublas_handle)

from .          import core, cuda, data, hittable, material, engine
from .core      import Vector2, Vector3, Color, Matrix3, Transform
from .host      import CUDAMemInfo, CUDAProfiler
from .material  import Material, Texture
from .hittable  import Hittable, Volumetric, HitRecord
from .interface import SceneInterface

from .engine import *
from .data   import *

