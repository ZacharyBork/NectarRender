import _pathtracer
root = _pathtracer.material
tex  = root.texture

import numpy as np
from os      import PathLike
from pathlib import Path
from PIL     import Image
from typing  import TypeAlias

from nectar_render.python.core import Color


MaterialType: TypeAlias = root.MaterialType
TextureType:  TypeAlias = tex.TextureType

###############################################################################
# TEXTURES
###############################################################################

class Texture(tex.Texture):
    
    @staticmethod
    def from_color(
        color: Color | tuple[float, float, float] = (1.0, 1.0, 1.0)
    ) -> tex.Texture:
        if isinstance(color, _pathtracer.core.vector.Color): 
            return tex.Texture.from_color(color)
        return tex.Texture.from_color(*color)
    
    @staticmethod
    def from_image(filepath: PathLike, scale: float = 1.0) -> tex.Texture:
        fp = Path(filepath)
        if fp.exists(): return tex.Texture.from_image(fp.as_posix(), scale)
        raise FileNotFoundError(
            f'Unable to locate image file at path: {fp.as_posix()}'
        )
        
        

###############################################################################
# MATERIAL
###############################################################################

class Material(root.Material):
    
    # CONSTRUCTORS ############################################################

    @staticmethod
    def LAMBERTIAN(
        albedo: Texture | Color = Color(0.8, 0.8, 0.8)
    ) -> root.Material:
        return root.Material.lambertian(albedo)
    
    @staticmethod
    def PBR(
        albedo:    PathLike | Texture | Color = Color(1.0, 1.0, 1.0),
        roughness: PathLike | Texture | float = 0.8,
        metallic:  PathLike | Texture | float = 0.0,
        emission:  PathLike | Texture | Color = Color(0.0, 0.0, 0.0),
        normal:    PathLike | Texture | None = None
    ) -> root.Material:
        albedo    = Material._to_texture(albedo)
        roughness = Material._to_texture(roughness)
        metallic  = Material._to_texture(metallic)
        emission  = Material._to_texture(emission)
        normal    = Material._to_texture(normal, Color(0.5, 0.5, 1.0))
                
        return root.Material.pbr(
            albedo, roughness, metallic, emission, normal
        )
        
    @staticmethod
    def DIELECTRIC(ior: float = 1.5) -> root.Material:
        return root.Material.dielectric(ior)
    
    @staticmethod
    def EMISSIVE(
        albedo: Texture | Color = Color(1.0, 1.0, 1.0),
        brightness: float = 35.0
    ) -> root.Material:
        return root.Material.emissive(albedo, brightness)
    
    @staticmethod
    def ISOTROPIC(
        albedo: Texture | Color = Color(0.8, 0.8, 0.8)
    ) -> root.Material:
        return root.Material.isotropic(albedo)


    # UTILITIES ###############################################################

    @staticmethod
    def _to_texture(
        input:    PathLike | Texture | Color | float | None,
        fallback: Color = Color(0.0, 0.0, 0.0)
    ) -> tex.Texture:
        if input is None: return Texture.from_color(fallback)
        
        if isinstance(input, tex.Texture): return input
        if isinstance(input, float): return Texture.from_color(Color(input))
        if isinstance(input, _pathtracer.core.vector.Color):
            return Texture.from_color(input)
        
        path = Path(input).resolve()
        if path.exists(): return Texture.from_image(path)
        raise FileNotFoundError(
            f'Unable to locate texture at path: {path.as_posix()}'
        )
        


