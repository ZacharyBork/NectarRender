"""
Core module.
"""
from __future__ import annotations
import typing
from . import vector
__all__: list[str] = ['allocate_cuda_memory', 'destroy_cublas_handle', 'free_cuda_memory', 'vector']
def allocate_cuda_memory(arg0: typing.SupportsInt | typing.SupportsIndex, arg1: typing.SupportsFloat | typing.SupportsIndex) -> int:
    ...
def destroy_cublas_handle() -> None:
    """
    Destroys cuBLAS handle. Registered atexit for Python module.
    """
def free_cuda_memory(arg0: typing.SupportsInt | typing.SupportsIndex) -> None:
    ...
