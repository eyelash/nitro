uniform mat4 projection;
attribute vec4 vertex;
attribute vec4 color;

varying vec4 v_color;

void main () {
	gl_Position = projection * vertex;
	v_color = color;
}
