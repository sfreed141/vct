# Voxelize

Implementation of voxelization portion of Voxel Cone Tracing.

## References
* [Interactive Indirect Illumination Using Voxel Cone Tracing](https://research.nvidia.com/publication/interactive-indirect-illumination-using-voxel-cone-tracing)
* [OpenGL Insights: Octree-Based Sparse Voxelization Using the GPU Hardware Rasterizer](https://www.seas.upenn.edu/~pcozzi/OpenGLInsights/OpenGLInsights-SparseVoxelization.pdf)

## Libraries Used
* [GLEW](http://glew.sourceforge.net/)
* [GLFW](http://www.glfw.org/)
* [GLM](https://glm.g-truc.net/0.9.8/index.html)
* [stb_image.h](https://github.com/nothings/stb)
* [tiny_obj_loader.h](https://github.com/syoyo/tinyobjloader)
* [nuklear](https://github.com/vurtun/nuklear)

## Models
* [Sponza](http://casual-effects.com/data/index.html)

## Building
### Linux
1. Install glm, glfw3, and glew using your package manager.
2. `cd` into root directory of project. `mkdir build; cd build; cmake ..; make`.
3. Good to go!

### Windows
1. Install Visual Studio Community 2017. Make sure to install Visual C++ and enable the "Desktop development with C++" option.
2. Clone (or download) [vcpkg](https://github.com/Microsoft/vcpkg) and follow installation directions. Then install glm, glfw3, and glew.
3. Update the CMAKE_TOOLCHAIN_FILE variable in the CMakeSettings.json file.
4. Good to go!