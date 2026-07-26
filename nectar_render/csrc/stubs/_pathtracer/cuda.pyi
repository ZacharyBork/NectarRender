"""
CUDA utility module.
"""
from __future__ import annotations
__all__: list[str] = ['cudaDeviceSynchronize', 'destroy_cublas_handle']
def cudaDeviceSynchronize() -> None:
    ...
def destroy_cublas_handle() -> None:
    """
    Destroys cuBLAS handle. Registered atexit for Python module.
    """
