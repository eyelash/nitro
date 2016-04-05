precision mediump float;

uniform sampler2D texture;
varying vec4 v_texcoord;

void main () {
	gl_FragColor = texture2D (texture, v_texcoord.xy);
}
