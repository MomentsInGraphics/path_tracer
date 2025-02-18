#include "camera.h"
#include "math_utilities.h"
#include "timer.h"
#include <math.h>
#include <string.h>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>


void control_rotation(controllable_rotation_t* rotation, GLFWwindow* window) {
	// Defines which mouse axis controls which rotation angle and the rotation
	// speed (in radians per pixel)
	const float rot_speed = M_PI / 2000.0f;
	const float mouse_offset_to_rotation[3 * 2] = {
		0.0f, -rot_speed,
		0.0f, 0.0f,
		rot_speed, 0.0f,
	};
	// Query the mouse cursor position
	double mouse_pos_d[2] = { 0.0, 0.0 };
	glfwGetCursorPos(window, &mouse_pos_d[0], &mouse_pos_d[1]);
	float mouse_pos[2] = { (float) mouse_pos_d[0], (float) mouse_pos_d[1] };
	// Activate/deactivate mouse rotation
	int rmb_state = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_2);
	if (!rotation->mouse_active && rmb_state == GLFW_PRESS) {
		rotation->mouse_active = true;
		float angles_offset[3];
		mat_vec_mul(angles_offset, mouse_offset_to_rotation, mouse_pos, 3, 2);
		for (uint32_t i = 0; i != 3; ++ i)
			rotation->origin_angles[i] = rotation->angles[i] - angles_offset[i];
	}
	else if (rotation->mouse_active && rmb_state == GLFW_RELEASE)
		rotation->mouse_active = false;
	// Update the rotation
	if (rotation->mouse_active) {
		float angles_offset[3];
		mat_vec_mul(angles_offset, mouse_offset_to_rotation, mouse_pos, 3, 2);
		for (uint32_t i = 0; i != 3; ++ i)
			rotation->angles[i] = rotation->origin_angles[i] + angles_offset[i];
	}
	// Limit rotation along the x-axis
	if (rotation->angles[0] < 0.0f)
		rotation->angles[0] = 0.0f;
	if (rotation->angles[0] > M_PI)
		rotation->angles[0] = M_PI;
}


void control_camera(camera_t* camera, GLFWwindow* window) {
	control_rotation(&camera->rotation, window);
	// Movement happens along three axes using WASD and QE
	float x = 0.0f, y = 0.0f, z = 0.0f;
	if (glfwGetKey(window, GLFW_KEY_W)) y += 1.0f;
	if (glfwGetKey(window, GLFW_KEY_A)) x -= 1.0f;
	if (glfwGetKey(window, GLFW_KEY_S)) y -= 1.0f;
	if (glfwGetKey(window, GLFW_KEY_D)) x += 1.0f;
	if (glfwGetKey(window, GLFW_KEY_Q)) z -= 1.0f;
	if (glfwGetKey(window, GLFW_KEY_E)) z += 1.0f;
	// Apply the motion along the right axes depending on the camera type
	float offset[3];
	float log_height_factor = 0.0f;
	switch (camera->type) {
		case camera_type_first_person:
		case camera_type_hemispherical:
		case camera_type_spherical:
		{
			// WASD move in the xy-plane and QE along the z-axis
			float sin_z = sinf(camera->rotation.angles[2]);
			float cos_z = cosf(camera->rotation.angles[2]);
			offset[0] = -cos_z * x - sin_z * y;
			offset[1] = sin_z * x - cos_z * y;
			offset[2] = z;
			break;
		}
		case camera_type_ortho:
		{
			float local_offset[3] = { x, -y, 0.0f };
			float rotation[3 * 3];
			rotation_matrix_from_angles(rotation, camera->rotation.angles);
			mat_vec_mul(offset, rotation, local_offset, 3, 3);
			log_height_factor = 0.1f * z;
			break;
		}
		default:
			offset[0] = offset[1] = offset[2] = 0.0f;
			break;
	}
	// Apply the motion
	float final_speed = camera->speed;
	if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT))
		final_speed *= 10.0f;
	if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL))
		final_speed *= 0.1f;
	float step = final_speed * get_frame_delta();
	if (normalize(offset, 3))
		for (uint32_t i = 0; i != 3; ++i)
			camera->position[i] += step * offset[i];
	camera->height *= expf(step * log_height_factor);
}


void get_world_to_view_space(float world_to_view[4 * 4], const camera_t* camera) {
	// Construct the view to world space rotation matrix
	float rotation[3 * 3];
	rotation_matrix_from_angles(rotation, camera->rotation.angles);
	// We actually want the world to view space, so we transform the position
	// and work with the transpose (i.e. inverse) rotation matrix
	float translation[3];
	mat_mat_mul(translation, camera->position, rotation, 1, 3, 3);
	float new_world_to_view[4 * 4] = {
		rotation[0 * 3 + 0], rotation[1 * 3 + 0], rotation[2 * 3 + 0], -translation[0],
		rotation[0 * 3 + 1], rotation[1 * 3 + 1], rotation[2 * 3 + 1], -translation[1],
		rotation[0 * 3 + 2], rotation[1 * 3 + 2], rotation[2 * 3 + 2], -translation[2],
		0.0f, 0.0f, 0.0f, 1.0f,
	};
	memcpy(world_to_view, new_world_to_view, sizeof(new_world_to_view));
}


void get_view_to_projection_space(float view_to_projection[4 * 4], const camera_t* camera, float aspect_ratio) {
	float near = camera->near;
	float far = camera->far;
	switch (camera->type) {
		case camera_type_first_person:
		case camera_type_hemispherical:
		case camera_type_spherical:
		{
			float top = tanf(0.5f * camera->fov);
			float right = aspect_ratio * top;
			float new_view_to_projection[4 * 4] = {
				-1.0f / right, 0.0f, 0.0f, 0.0f,
				0.0f, 1.0f / top, 0.0f, 0.0f,
				0.0f, 0.0f, (far + near) / (near - far), 2.0f * far * near / (near - far),
				0.0f, 0.0f, -1.0f, 0.0f,
			};
			memcpy(view_to_projection, new_view_to_projection, sizeof(new_view_to_projection));
			break;
		}
		case camera_type_ortho:
		{
			float height = camera->height;
			float width = aspect_ratio * height;
			// Based on: https://registry.khronos.org/OpenGL-Refpages/gl2.1/xhtml/glOrtho.xml
			float new_view_to_projection[4 * 4] = {
				2.0f / width, 0.0f, 0.0f, 0.0f,
				0.0f, 2.0f / height, 0.0f, 0.0f,
				0.0f, 0.0f, -2.0f / (far - near), -(far + near) / (far - near),
				0.0f, 0.0f, 0.0f, 1.0f,
			};
			memcpy(view_to_projection, new_view_to_projection, sizeof(new_view_to_projection));
			break;
		}
		default:
			memset(view_to_projection, 0, sizeof(float) * 4 * 4);
			break;
	}
}


void get_world_to_projection_space(float world_to_projection[4 * 4], const camera_t* camera, float aspect_ratio) {
	// Retrieve and combine the relevant transforms
	float world_to_view[4 * 4];
	float view_to_projection[4 * 4];
	get_world_to_view_space(world_to_view, camera);
	get_view_to_projection_space(view_to_projection, camera, aspect_ratio);
	mat_mat_mul(world_to_projection, view_to_projection, world_to_view, 4, 4, 4);
}
