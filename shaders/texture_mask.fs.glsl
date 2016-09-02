precision mediump float;

uniform sampler2D texture;
uniform sampler2D mask;
varying vec4 v_texcoord;
varying vec4 v_mask_texcoord;

void main () {
	gl_FragColor = texture2D (texture, v_texcoord.xy) * vec4 (1.0, 1.0, 1.0, texture2D (mask, v_mask_texcoord.xy).a);
}
