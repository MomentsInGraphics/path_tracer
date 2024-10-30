#pragma once
#include <stdbool.h>


//! Forward declaration to be able to access mouse and keyboard input
typedef struct GLFWwindow GLFWwindow;


//! Describes a 3D rotation along with attributes needed to implement mouse
//! control
typedef struct {
	//! The current view- to world-space rotation in the form expected by
	//! rotation_matrix_from_angles()
	float angles[3];
	//! Whether the camera rotation is currently being controlled by the mouse
	//! cursor
	bool mouse_active;
	//! While mouse rotation is active, these are the values that
	//! rotation_angles would have if the mouse were at pixel (0, 0)
	float origin_angles[3];
} controllable_rotation_t;


//! Lists available types of cameras with different projections and controls
typedef enum {
	//! A first-person free-flight camera with perspective projection
	camera_type_first_person,
	//! A camera with orthographic projection and controls similar to the
	//! first person camera
	camera_type_ortho,
	//! A camera that uses spherical coordinates to show an entire hemisphere
	camera_type_hemispherical,
	//! A camera that uses spherical coordinates to show an entire sphere
	camera_type_spherical,
	//! Number of entries in this enumeration
	camera_type_count,
} camera_type_t;


//! A camera with orthographic projection and controls similar to the
//! camera_first_person
typedef struct {
	//! The rotation of the camera
	controllable_rotation_t rotation;
	/*! The world space position of the camera. For perspective projections,
		this is the point of view, for orthographic projections it is a point
		that will be shown in the middle of the viewport.*/
	float position[3];
	//! The base speed of the position in world-space distance per second
	float speed;
	//! The signed distance of the near and far clipping plane to position.
	//! Usually near < far.
	float near, far;
	//! The type of this camera. Affects the kind of projection and the
	//! position controls.
	camera_type_t type;
	//! The vertical field of view in radians of a perspective camera, i.e. the
	//! angle between the top and bottom clipping planes
	float fov;
	//! The world-space distance between the top and bottom clipping plane for
	//! orthographic cameras
	float height;
} camera_t;


//! Retrieves mouse input from the given window and updates the given rotation
//! accordingly
void control_rotation(controllable_rotation_t* rotation, GLFWwindow* window);


//! Retrieves keyboard and mouse input from the given window and updates the
//! given camera accordingly. Requires regular calls to record_frame_time().
void control_camera(camera_t* camera, GLFWwindow* window);


//! Like get_world_to_projection_space() but for world to view space
void get_world_to_view_space(float world_to_view[4 * 4], const camera_t* camera);


//! Like get_world_to_projection_space() but for view to projection space
void get_view_to_projection_space(float view_to_projection[4 * 4], const camera_t* camera, float aspect_ratio);


/*! Constructs the world to projection space transformation matrix to be used
	for the given camera.
	\param world_to_projection Output array for a 4x4 matrix in row-major
		layout, which is to be multiplied onto world-space colum vectors from
		the left.
	\param camera The camera for which to construct the transform.
	\param aspect_ratio The ratio of viewport width over viewport height.*/
void get_world_to_projection_space(float world_to_projection[4 * 4], const camera_t* camera, float aspect_ratio);
