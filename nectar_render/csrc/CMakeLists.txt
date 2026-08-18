##############################################################################
# INIT CMAKE PROJECT
##############################################################################

cmake_minimum_required(VERSION 3.18)
find_program(CMAKE_CUDA_COMPILER nvcc
    HINTS
        $ENV{CUDA_HOME}
        $ENV{CUDA_PATH}
        /usr/local/cuda/bin
)
project(nectar_render LANGUAGES CXX CUDA)

##############################################################################
# C++ / CUDA STANDARDS
##############################################################################

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CUDA_STANDARD 17)
set(CMAKE_CUDA_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(OpenImageDenoise_DIR "$ENV{OIDN_DIR}")

##############################################################################
# CLI OVERRIDE FOR CUDA ARCHITECTURE
##############################################################################
# pip install . --config-settings=cmake.args="-DCUDA_ARCHITECTURES=70;75;80;"

set(CUDA_ARCHITECTURES "75" CACHE STRING "CUDA architectures to target")

##############################################################################
# FIND DEPENDENCIES
##############################################################################

find_package(pybind11 REQUIRED)
find_package(CUDAToolkit REQUIRED)
find_package(CUDA REQUIRED)
find_package(OpenImageDenoise REQUIRED)

##############################################################################
# GATHER SOURCES
##############################################################################

file(GLOB_RECURSE CUDA_SOURCES "*.cu")
file(GLOB_RECURSE CXX_SOURCES "*.cpp")

##############################################################################
# ADD LIBRARIES / MODULES
##############################################################################

add_library(nectar_cuda STATIC ${CUDA_SOURCES})
pybind11_add_module(_pathtracer bindings/bindings.cpp ${CXX_SOURCES})

##############################################################################
# SET PACKAGE PROPERTIES
##############################################################################

set_target_properties(nectar_cuda PROPERTIES
    CUDA_RESOLVE_DEVICE_SYMBOLS ON
    CUDA_ARCHITECTURES ${CUDA_ARCHITECTURES}
    POSITION_INDEPENDENT_CODE ON
)

set_target_properties(_pathtracer 
    PROPERTIES CUDA_ARCHITECTURES 
    ${CUDA_ARCHITECTURES}
    BUILD_RPATH "/opt/oidn/lib"
    INSTALL_RPATH "/opt/oidn/lib"
)

##############################################################################
# LINK LIBRARIES
##############################################################################

target_link_libraries(nectar_cuda PRIVATE 
    CUDA::cudart 
    cublas 
    atomic
    OpenImageDenoise
)
target_link_libraries(_pathtracer PRIVATE
    nectar_cuda
    CUDA::cudart
    cublas
    atomic
    OpenImageDenoise
)

##############################################################################
# SETUP PACKAGE INCLUDES
##############################################################################

target_include_directories(_pathtracer PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}
    external/tinyobjloader
    external/stb
)
target_include_directories(nectar_cuda PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${pybind11_INCLUDE_DIRS}
    external/tinyobjloader
    external/stb
)

##############################################################################
# COMPILE OPTIONS
##############################################################################

target_compile_options(nectar_cuda PRIVATE
    $<$<COMPILE_LANGUAGE:CUDA>:--use_fast_math>
)

# # Uncomment to include ptxas output in build log.
# target_compile_options(nectar_cuda PRIVATE
#     $<$<COMPILE_LANGUAGE:CUDA>:-Xptxas -v>
# )

##############################################################################
# INSTALL PACKAGE
##############################################################################

install(TARGETS _pathtracer DESTINATION .)

##############################################################################
# GENERATE PYBIND11 STUBS
##############################################################################
# make -C build stubgen

find_program(STUBGEN_EXECUTABLE pybind11-stubgen)
add_custom_target(stubgen
    COMMAND ${CMAKE_COMMAND} -E env
            "PYTHONPATH=$<TARGET_FILE_DIR:_pathtracer>"
            ${STUBGEN_EXECUTABLE} _pathtracer --output-dir ${CMAKE_SOURCE_DIR}/stubs
    DEPENDS _pathtracer
    COMMENT "Generating pybind11 stubs for _pathtracer"
)
