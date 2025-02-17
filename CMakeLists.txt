cmake_minimum_required (VERSION 3.11)

# We build a single executable
project(path_tracer)
add_executable(path_tracer)
target_compile_definitions(path_tracer
	# Keep OpenGL from being included
	PUBLIC GLFW_INCLUDE_NONE
	# Silence MS build warnings
	PUBLIC _CRT_SECURE_NO_WARNINGS
)

# We use C99
set_target_properties(path_tracer PROPERTIES C_STANDARD 99)
set_target_properties(path_tracer PROPERTIES CMAKE_C_STANDARD_REQUIRED True)

# All source code (except dependencies) is in src
add_subdirectory(src)

# Add Vulkan and GLFW as dependencies
find_package(glfw3 3.3)
if(NOT ${glfw3_FOUND})
	set(GLFW_BUILD_DOCS False)
	set(GLFW_BUILD_EXAMPLES False)
	set(GLFW_BUILD_TESTS False)
	set(GLFW_VULKAN_STATIC True)
	add_subdirectory(ext/glfw)
endif()
find_package(Vulkan REQUIRED)
target_link_libraries(path_tracer PRIVATE Vulkan::Vulkan glfw)

# math.h is being used
if (UNIX)
	find_library(MATH_LIBRARY m)
	if(MATH_LIBRARY)
		target_link_libraries(path_tracer PUBLIC ${MATH_LIBRARY})
		# MSVC does not support variable-length arrays or void* pointer arithmetic,
		# so we always treat these things as errors in Linux
		set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Werror=vla -Werror=pointer-arith")
	endif()
endif()
