import _pathtracer
root = _pathtracer.material
tex  = root.texture

import numpy as np
from os      import PathLike
from typing  import Self, TypeAlias
from pathlib import Path
from PIL     import Image

from nectar_render.python.core import Color

###############################################################################
# TEXTURES
###############################################################################

Texture: TypeAlias = tex.Texture

class ImageTexture(tex.ImageTexture):
    def __init__(
        self: Self,
        fp:   PathLike    
    ) -> None:
        fp = Path(fp)
        if not fp.exists():
            raise FileNotFoundError(
                f'Unable to locate image file at path: {fp.as_posix()}'
            )
        
        img = Image.open(fp).convert('RGB')
        arr = np.array(img, dtype=np.uint8).transpose((2, 0, 1))
        ptr = np.ascontiguousarray(arr).ctypes.data
        C, H, W = arr.shape
        
        super().__init__(ptr, C, H, W)

###############################################################################
# MATERIAL
###############################################################################

class Material:
    
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
    ) -> tex.ConstantTexture | ImageTexture:
        if input is None: return tex.ConstantTexture(fallback)
        
        if isinstance(input, Texture): return input
        if isinstance(input, float): return tex.ConstantTexture(Color(input))
        if isinstance(input, _pathtracer.core.vector.Color):
            return tex.ConstantTexture(input)
        
        path = Path(input).resolve()
        if path.exists(): return ImageTexture(path)
        raise FileNotFoundError(
            f'Unable to locate texture at path: {path.as_posix()}'
        )
        


