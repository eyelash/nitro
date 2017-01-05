precision mediump float;

uniform sampler2D mask;
varying vec4 v_color;
varying vec4 v_texcoord;

void main() {
	gl_FragColor = v_color * vec4(1.0, 1.0, 1.0, texture2D(mask, v_texcoord.xy).a);
}
