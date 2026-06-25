import _pathtracer

def cuda_synchronize() -> None:
    _pathtracer.core.utils.cuda_synchronize()

