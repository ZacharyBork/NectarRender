import _pathtracer
root = _pathtracer.core

from typing import Self

###############################################################################
# VECTOR
###############################################################################

class Vector2(root.vector.Vector2):
    def __repr__(self: Self) -> str:
        return f'Vector2(x={self.x()}, y={self.y()})'
    
Vec2 = Point2 = Vector2;

class Vector3(root.vector.Vector3):
    def __repr__(self: Self) -> str:
        return f'Vector3(x={self.x()}, y={self.y()}, z={self.z()})'

    def as_tuple(self: Self) -> tuple[float, float, float]:
        return (self.x(), self.y(), self.z())

Vec3 = Point3 = Vector3;

class Color(root.vector.Color):
    def __repr__(self: Self) -> str:
        return f'Color(r={self.r()}, g={self.g()}, b={self.b()})'

###############################################################################
# MATRIX
###############################################################################

class Matrix3(root.matrix.Matrix3):
    @staticmethod
    def rotation_from_euler(r: Vector3) -> Self:
        return root.matrix.rotation_from_euler(r)

###############################################################################
# TRANSFORM
###############################################################################

class Transform(root.Transform): ...

