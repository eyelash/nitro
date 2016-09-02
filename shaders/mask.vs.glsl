uniform mat4 projection;
attribute vec4 vertex;
attribute vec4 color;
attribute vec4 texcoord;

varying vec4 v_color;
varying vec4 v_texcoord;

void main () {
	gl_Position = projection * vertex;
	v_color = color;
	v_texcoord = texcoord;
}
