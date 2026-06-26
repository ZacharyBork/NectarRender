import _pathtracer

class Material(_pathtracer.material.Material):
    LAMBERTIAN = _pathtracer.material.Lambertian
    METAL      = _pathtracer.material.Metal
    DIELECTRIC = _pathtracer.material.Dielectric

class Texture(_pathtracer.material.texture.Texture):
    CONSTANT = _pathtracer.material.texture.ConstantTexture
    CHECKER  = _pathtracer.material.texture.CheckerTexture


