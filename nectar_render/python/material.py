import _pathtracer
root = _pathtracer.material

import numpy as np
from os      import PathLike
from typing  import Self
from pathlib import Path
from PIL     import Image

###############################################################################
# WRAPPERS
###############################################################################

class ImageTexture(root.texture.ImageTexture):
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

class Material(root.Material):
    LAMBERTIAN = root.Lambertian
    METAL      = root.Metal
    DIELECTRIC = root.Dielectric
    EMISSIVE   = root.Emissive
    

class Texture(root.texture.Texture):
    CONSTANT = root.texture.ConstantTexture
    CHECKER  = root.texture.CheckerTexture
    NOISE    = root.texture.NoiseTexture
    IMAGE    = ImageTexture


