"""
NectarRender C++ host module.
"""
from __future__ import annotations
import numpy
import typing
__all__: list[str] = ['allocate_cuda_memory', 'destroy_cublas_handle', 'device_to_cpu', 'free_cuda_memory', 'test_fn']
def allocate_cuda_memory(arg0: typing.SupportsInt | typing.SupportsIndex, arg1: typing.SupportsFloat | typing.SupportsIndex) -> int:
    ...
def destroy_cublas_handle() -> None:
    """
    Destroys cuBLAS handle. Registered atexit for Python module.
    """
def device_to_cpu(arg0: typing.SupportsInt | typing.SupportsIndex, arg1: ..., std: ..., arg2: bool) -> numpy.ndarray:
    ...
def free_cuda_memory(arg0: typing.SupportsInt | typing.SupportsIndex) -> None:
    ...
def test_fn() -> None:
    ...
