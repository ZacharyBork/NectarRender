"""
Interface module.
"""
from __future__ import annotations
import _pathtracer.hittable
import _pathtracer.material
__all__: list[str] = ['ObjectInterface']
class ObjectInterface:
    def get_material(self) -> _pathtracer.material.Material:
        ...
    def hit_record(self) -> _pathtracer.hittable.HitRecord:
        ...
    def is_enabled(self) -> bool:
        ...
    def update_material(self, mat: _pathtracer.material.Material) -> None:
        ...
