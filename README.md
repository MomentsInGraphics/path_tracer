This repository contains a Vulkan rendering framework with an implementation of a path tracer written for educational purposes.
The core code of the path tracer can be found in src/shaders/pathtrace.frag.glsl.
The underlying rendering algorithms are described in lectures available here:
https://graphics.tudelft.nl/path_tracing_lectures


# Build instructions

Dependencies of this project are GLFW3 and Vulkan and those have to be installed separately.
For Windows the Vulkan SDK can be downloaded here:
https://vulkan.lunarg.com/sdk/home
And GLFW is available here:
https://www.glfw.org/download.html
Download the GLFW source package, unzip it into ext and rename it such that the path ext/glfw/CMakeLists.txt is valid.

For Linux, it is probably easier to obtain Vulkan and GLFW via a package manager.
For pacman, the relevant packages are: vulkan-headers vulkan-validation-layers glfw

Once all dependencies are available, use CMake to build the project using your preferred IDE or the following commands (using the directory containing this README.md as working directory):
$ cmake .
$ make


# Run instructions

If you want to see a scene other than the Cornell box, you must first make sure that you have all data files upon which the renderer depends.
Several large files are not part of this repository and should instead be downloaded at the following link and added to the data directory.
https://graphics.tudelft.nl/path_tracing_lectures

Then launch the path_tracer binary.
**Important:** The current working directory has to be the directory containing this README.md.
If anything goes wrong, there should be an informative error message on stdout.

Camera controls use WASD for horizontal movement, QE to move down/up and control/shift to adjust the speed. F1 toggles the UI, F2 toggles v-sync, F3/F4 are quicksave/quickload, F5 reloads shaders, holding F6 disables sample accumulation and F10/F11/F12 store screenshots to data in various formats.