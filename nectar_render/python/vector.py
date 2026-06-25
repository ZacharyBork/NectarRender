import _pathtracer

from typing import Self

class Vector2(_pathtracer.core.vector.Vector2):
    def __repr__(self: Self) -> str:
        return f'Vector2(x={self.x()}, y={self.y()})'

class Vector3(_pathtracer.core.vector.Vector3):
    def __repr__(self: Self) -> str:
        return f'Vector3(x={self.x()}, y={self.y()}, z={self.z()})'

class Color(_pathtracer.core.vector.Color):
    def __repr__(self: Self) -> str:
        return f'Color(r={self.r()}, g={self.g()}, b={self.b()})'
