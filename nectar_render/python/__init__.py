import atexit
import _pathtracer
atexit.register(_pathtracer.core.destroy_cublas_handle)

from .engine import RenderEngine
from .camera import Camera
from .vector import Vector2, Vector3, Color


