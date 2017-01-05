precision mediump float;

varying vec4 v_color;

vec4 unpremultiply(in vec4 c) {
	return c.a == 0.0 ? vec4(0.0) : vec4(c.rgb / vec3(c.a), c.a);
}

void main() {
	gl_FragColor = unpremultiply(v_color);
}
