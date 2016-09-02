precision mediump float;

uniform sampler2D texture;
uniform float alpha;
varying vec4 v_texcoord;

void main () {
	gl_FragColor = texture2D (texture, v_texcoord.xy) * vec4 (1.0, 1.0, 1.0, alpha);
}
