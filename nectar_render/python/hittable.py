import _pathtracer
root = _pathtracer.hittable

from os import PathLike
from typing import TypeAlias
from nectar_render.python.core import Color, Vector3

HitRecord: TypeAlias = root.HitRecord
Material:  TypeAlias = _pathtracer.material.Material

class Hittable:
    
    @staticmethod
    def QUAD(
        position: Vector3  = Vector3(0.0, 0.0, 0.0),
        rotation: Vector3  = Vector3(0.0, 0.0, 0.0),
        scale:    Vector3  = Vector3(0.0, 0.0, 0.0),
        material: Material = Material.lambertian(Color(1.0, 0.0, 1.0))
    ) -> root.Hittable:
        return root.Hittable.quad(position, rotation, scale, material)
    
    @staticmethod
    def CUBE(
        position: Vector3  = Vector3(0.0, 0.0, 0.0),
        rotation: Vector3  = Vector3(0.0, 0.0, 0.0),
        scale:    Vector3  = Vector3(0.0, 0.0, 0.0),
        material: Material = Material.lambertian(Color(1.0, 0.0, 1.0))
    ) -> root.Hittable:
        return root.Hittable.cube(position, rotation, scale, material)
    
    @staticmethod
    def SPHERE(
        position: Vector3  = Vector3(0.0, 0.0, 0.0),
        radius:   float    = 1.0,
        material: Material = Material.lambertian(Color(1.0, 0.0, 1.0))
    ) -> root.Hittable:
        return root.Hittable.sphere(position, radius, material)
    
    @staticmethod
    def MESH(
        filepath: PathLike,
        position: Vector3  = Vector3(0.0, 0.0, 0.0),
        rotation: Vector3  = Vector3(0.0, 0.0, 0.0),
        scale:    Vector3  = Vector3(0.0, 0.0, 0.0),
        material: Material = Material.lambertian(Color(1.0, 0.0, 1.0))
    ) -> root.Hittable:
        return root.Hittable.mesh(
            filepath, position, rotation, scale, material
        )
        
    @staticmethod
    def OBJECT_LIGHT(
        bound_object: root.Hittable,
        brightness: float = 1.0,
        albedo: Color = Color(1.0, 1.0, 1.0)
    ) -> root.Hittable:
        return root.Hittable.object_light(bound_object, brightness, albedo)
    
    
class Volumetric:
    
    @staticmethod
    def CONSTANT(
        bound_object: root.Hittable,
        density: float = 1.0,
        albedo: Color = Color(1.0, 1.0, 1.0)
    ) -> root.Hittable:
        return root.Hittable.constant_medium(bound_object, density, albedo)



