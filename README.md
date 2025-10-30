This repository contains supplemental code for the paper "[Jackknife Transmittance and MIS Weight Estimation](https://momentsingraphics.de/SiggraphAsia2025.html)" published at SIGGRAPH Asia 2025.


# Build instructions

Dependencies of this project are GLFW3 and Vulkan and those have to be installed separately.
For Windows, the Vulkan SDK can be downloaded here:
https://vulkan.lunarg.com/sdk/home
And GLFW is available here:
https://www.glfw.org/download.html
Download its source package, unzip it into ext and rename it such that the path ext/glfw/CMakeLists.txt is valid.

For Linux, it is probably easier to obtain Vulkan and GLFW via a package manager.
For pacman, the relevant packages are: vulkan-headers vulkan-validation-layers glfw

Once all dependencies are available, use CMake to build the project using your preferred IDE or the following commands (using the directory containing this README.md as working directory):
```
$ cmake .
$ make
```


# Run instructions

First make sure that you have all data files upon which the renderer depends.
Several large files are not part of this repository and should instead be downloaded at the following link:
https://momentsingraphics.de/Media/SiggraphAsia2025/peters2025-jackknife-code.zip

Then launch the volume_renderer binary.
**Important:** The current working directory has to be the directory containing this README.md.
If anything goes wrong, there should be an informative error message on stdout.

Camera controls use WASD for horizontal movement, QE to move down/up, right mouse button to rotate the camera and control/shift to adjust the speed. F1 toggles the UI, F2 toggles v-sync, F3/F4 are quicksave/quickload, F5 reloads shaders, holding F6 disables sample accumulation and F10/F11/F12 store screenshots to the data folder using hdr/png/jpg as format.

The following command will populate data/jackknife_screenshots with all screenshots used for creation of figures and supplemental materials in the paper.
It will also print timings.
Due to high sample counts in some of these experiments, this will take several hours, even on fast GPUs.
```
$ volume_renderer -b0 -e293 -no_v_sync
```


# Interesting code files

All implementations of transmittance estimators, MIS weight estimators and free-flight distance samplers are in src/shaders/volume_utilities.glsl.
The jackknife transmittance estimator is implemented in `estimate_volume_transmittance()` in this file.
Higher-level volume rendering code can be found in src/shaders/volume.frag.glsl.
That includes different techniques for multiple importance sampling in `ray_traced_volume_with_point_lights()`.
Evaluation and sampling of different phase functions is implemented in src/shaders/phase_functions.glsl.
All of the main logic of the application is found in src/main.c.
If you want to convert an OpenVDB file to the binary *.blob file format used by the renderer, you can use tools/volume_conversion.py.
