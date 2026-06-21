"""
Core module.
"""
from __future__ import annotations
import collections.abc
import numpy
import typing
__all__: list[str] = ['allocate_cuda_memory', 'destroy_cublas_handle', 'free_cuda_memory', 'to_numpy']
def allocate_cuda_memory(arg0: typing.SupportsInt | typing.SupportsIndex, arg1: typing.SupportsFloat | typing.SupportsIndex) -> int:
    ...
def destroy_cublas_handle() -> None:
    """
    Destroys cuBLAS handle. Registered atexit for Python module.
    """
def free_cuda_memory(arg0: typing.SupportsInt | typing.SupportsIndex) -> None:
    ...
def to_numpy(arg0: typing.SupportsInt | typing.SupportsIndex, arg1: collections.abc.Sequence[typing.SupportsInt | typing.SupportsIndex]) -> numpy.ndarray:
    ...
