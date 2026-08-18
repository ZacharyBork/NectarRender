import _pathtracer
root = _pathtracer.host

from typing import TypeAlias

###############################################################################
# PROFILING UTILITIES
###############################################################################

CUDAMemInfo: TypeAlias = root.memory.CUDAMemInfo

class CUDAProfiler:

    @staticmethod
    def get_cuda_meminfo() -> CUDAMemInfo: 
        return root.memory.get_cuda_meminfo()
    
    @staticmethod
    def get_cuda_memory_used() -> int:
        return root.memory.get_cuda_memory_used()
    
    @staticmethod
    def get_cuda_memory_free() -> int:
        return root.memory.get_cuda_memory_free()
    
    @staticmethod
    def get_cuda_memory_total() -> int:
        return root.memory.get_cuda_memory_total()


