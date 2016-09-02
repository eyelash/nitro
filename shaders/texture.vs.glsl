uniform mat4 projection;
attribute vec4 vertex;
attribute vec4 texcoord;

varying vec4 v_texcoord;

void main () {
	gl_Position = projection * vertex;
	v_texcoord = texcoord;
}
