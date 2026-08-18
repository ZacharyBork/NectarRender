# Temporary configuration file for NectarRender. Used to define scene, camera,
# etc. until saving/loading support is added.

from nectar_render.python import (
    Scene, Camera, Hittable, Material, Vector3, Color
)

MAX_DEPTH: int = 5
SEED:      int = 42

CAMERA = Camera(
    resolution   = (512, 512),
    position     = (0.0, 0.0, 2.0),
    rotation     = (0.0, 0.0, 0.0),
    num_samples  = 512,
    focal_length = 3.0
)

SCENE = Scene(
    skylight  = None,
    lights    = [
        Hittable.OBJECT_LIGHT(
            Hittable.QUAD(
                Vector3(0.0, 0.499, 0.0),
                Vector3(0.0, 180.0, 0.0),
                Vector3(0.2)
            ),
            35.0, Color.white()
        )
    ],
    hittables = [
        Hittable.QUAD( # Bottom
            Vector3(0.0, -0.5, 0.0),
            Vector3(0.0, 0.0, 0.0),
            Vector3(1.0),
            Material.LAMBERTIAN(Color.white())
        ),
        Hittable.QUAD( # Top
            Vector3(0.0, 0.5, 0.0),
            Vector3(0.0, 180.0, 0.0),
            Vector3(1.0),
            Material.LAMBERTIAN(Color.white())
        ),
        Hittable.QUAD( # Right
            Vector3(0.5, 0.0, 0.0),
            Vector3(0.0, 0.0, 90.0),
            Vector3(1.0),
            Material.LAMBERTIAN(Color.red())
        ),
        Hittable.QUAD( # Left
            Vector3(-0.5, 0.0, 0.0),
            Vector3(0.0, 0.0, -90.0),
            Vector3(1.0),
            Material.LAMBERTIAN(Color.green())
        ),
        Hittable.QUAD( # Back
            Vector3(0.0, 0.0, -0.5),
            Vector3(-90.0, 0.0, 0.0),
            Vector3(1.0),
            Material.LAMBERTIAN(Color.white())
        ),      
        Hittable.CUBE(
            Vector3(0.2, -0.35, 0.2), 
            Vector3(0.0, 35.0, 0.0),
            Vector3(0.3),
            Material.LAMBERTIAN(Color.white())
        ),
        Hittable.CUBE(
            Vector3(-0.2, -0.2, -0.1), 
            Vector3(0.0, 35.0, 0.0),
            Vector3(0.3, 0.6, 0.3),
            Material.LAMBERTIAN(Color.white())
        )
    ]
)


