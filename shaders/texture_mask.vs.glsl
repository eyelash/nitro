uniform mat4 projection;
attribute vec4 vertex;
attribute vec4 texcoord;
attribute vec4 mask_texcoord;

varying vec4 v_texcoord;
varying vec4 v_mask_texcoord;

void main () {
	gl_Position = projection * vertex;
	v_texcoord = texcoord;
	v_mask_texcoord = mask_texcoord;
}
