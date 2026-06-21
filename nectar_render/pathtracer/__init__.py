import atexit
import _pathtracer
atexit.register(_pathtracer.core.destroy_cublas_handle)

from .engine import ENGINE


