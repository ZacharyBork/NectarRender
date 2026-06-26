import _pathtracer
mat  = _pathtracer.material

import numpy as np
from os      import PathLike
from typing  import Self
from pathlib import Path
from PIL     import Image

###############################################################################
# WRAPPERS
###############################################################################

class ImageTexture(mat.texture.ImageTexture):
    def __init__(
        self: Self,
        fp:   PathLike    
    ) -> None:
        fp = Path(fp)
        if not fp.exists():
            raise FileNotFoundError(
                f'Unable to locate image file at path: {fp.as_posix()}'
            )
            
        arr = np.array(Image.open(fp), dtype=np.uint8).transpose((2, 0, 1))
        ptr = np.ascontiguousarray(arr).ctypes.data
        C, H, W = arr.shape
        
        super().__init__(ptr, C, H, W)

###############################################################################
# COLLECTIONS
###############################################################################

class Material(mat.Material):
    LAMBERTIAN = mat.Lambertian
    METAL      = mat.Metal
    DIELECTRIC = mat.Dielectric


class Texture(mat.texture.Texture):
    CONSTANT = mat.texture.ConstantTexture
    CHECKER  = mat.texture.CheckerTexture
    NOISE    = mat.texture.NoiseTexture
    IMAGE    = ImageTexture


