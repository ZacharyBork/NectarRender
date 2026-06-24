"""
Host module.
"""
from __future__ import annotations
import collections.abc
import numpy
import typing
__all__: list[str] = ['to_numpy']
def to_numpy(arg0: typing.SupportsInt | typing.SupportsIndex, arg1: collections.abc.Sequence[typing.SupportsInt | typing.SupportsIndex]) -> numpy.ndarray:
    ...
