import _pathtracer
root = _pathtracer.material
tex  = root.texture

import numpy as np
from os      import PathLike
from typing  import Self
from pathlib import Path
from PIL     import Image

from nectar_render.python.core import Color

###############################################################################
# TEXTURE WRAPPERS
###############################################################################

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
# MATERIAL WRAPPERS
###############################################################################

class PBRMaterial(root.PBRMaterial):
    def __init__(
        self:              Self,
        albedo:            PathLike | Color = Color(1.0, 1.0, 1.0),
        roughness:         PathLike | float = 0.8,
        metallic:          PathLike | float = 0.0,
        normal:            PathLike | None = None,
        normal_strength:   float = 1.0,
        ambient_occlusion: PathLike | float = 1.0,
        ao_power:          float = 1.0
    ) -> None:
                        
        albedo            = PBRMaterial._to_texture(albedo)
        roughness         = PBRMaterial._to_texture(roughness)
        metallic          = PBRMaterial._to_texture(metallic)
        ambient_occlusion = PBRMaterial._to_texture(ambient_occlusion)
         
        if normal is not None:   
              normal = PBRMaterial._image_texture(normal)
        else: normal = PBRMaterial._constant_texture(Color(0.0, 0.0, 1.0))
        
        super().__init__(
            albedo, roughness, metallic, normal, normal_strength,
            ambient_occlusion, ao_power
        )
        
    @staticmethod
    def _image_texture(path: PathLike) -> ImageTexture:
        path = Path(path).resolve()
        if not path.exists():
            raise FileNotFoundError(
                f'Unable to locate texture at path: {path.as_posix()}')
        return ImageTexture(path)
        
    @staticmethod
    def _to_texture(
        input: PathLike | Color | float
    ) -> tex.ConstantTexture | ImageTexture:
        if isinstance(input, _pathtracer.core.vector.Color):
            return tex.ConstantTexture(input)
        if isinstance(input, float):
            return tex.ConstantTexture(Color(input))
        return PBRMaterial._image_texture(input)
        

###############################################################################
# COLLECTIONS
###############################################################################

class Material(root.Material):
    LAMBERTIAN = root.Lambertian
    METAL      = root.Metal
    DIELECTRIC = root.Dielectric
    EMISSIVE   = root.Emissive
    PBR        = PBRMaterial
    

class Texture(tex.Texture):
    CONSTANT = tex.ConstantTexture
    CHECKER  = tex.CheckerTexture
    NOISE    = tex.NoiseTexture
    IMAGE    = ImageTexture


