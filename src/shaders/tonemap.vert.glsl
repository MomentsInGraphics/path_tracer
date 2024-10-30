#version 460

//! The x- and y-coordinates of the projection space position of the screen-
//! filling triangle
layout (location = 0) in vec2 g_vertex_pos;


void main() {
	gl_Position = vec4(g_vertex_pos, 0.0, 1.0);
}
