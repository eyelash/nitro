precision mediump float;

uniform bool use_texture;
uniform bool use_mask;
uniform bool use_inverted_mask;

uniform sampler2D texture;
uniform sampler2D mask;
uniform sampler2D inverted_mask;

varying vec4 v_color;
varying vec4 v_texture_texcoord;
varying vec4 v_mask_texcoord;
varying vec4 v_inverted_mask_texcoord;

void main() {
	if (use_texture) {
		gl_FragColor = texture2D(texture, v_texture_texcoord.xy);
	}
	else {
		gl_FragColor = v_color;
	}
	if (use_mask) {
		gl_FragColor *= vec4(1.0, 1.0, 1.0, texture2D(mask, v_mask_texcoord.xy).a);
	}
	if (use_inverted_mask) {
		gl_FragColor *= vec4(1.0, 1.0, 1.0, 1.0 - texture2D(inverted_mask, v_inverted_mask_texcoord.xy).a);
	}
}
