import _pathtracer
root = _pathtracer.hittable

class Hittable(root.Hittable):
    SPHERE = root.Sphere
    QUAD   = root.Quad
    CUBE   = root.Cube
    
class Volumetric(root.Hittable):
    CONSTANT = root.ConstantMedium



