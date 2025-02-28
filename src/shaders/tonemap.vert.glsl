#version 460


void main() {
	int id = gl_VertexIndex;
	vec2 pos = vec2(0.0);
	if (id == 0) pos = vec2(-1.1, -1.1);
	if (id == 1) pos = vec2(-1.1, 3.5);
	if (id == 2) pos = vec2(3.5, -1.1);
	gl_Position = vec4(pos, 0.0, 1.0);
}
