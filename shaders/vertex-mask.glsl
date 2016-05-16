uniform mat4 projection;
uniform mat4 clipping;
attribute vec4 vertex;
attribute vec4 color;
attribute vec4 texcoord;

varying vec4 v_color;
varying vec4 v_texcoord;
varying vec4 v_clipping;

void main () {
	gl_Position = projection * vertex;
	v_color = color;
	v_texcoord = texcoord;
	v_clipping = clipping * vertex;
}
