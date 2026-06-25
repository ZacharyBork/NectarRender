"""
Material module.
"""
from __future__ import annotations
import _pathtracer.core.vector
__all__: list[str] = ['Lambertian']
class Lambertian:
    def __init__(self, albedo: _pathtracer.core.vector.Color = ...) -> None:
        ...
