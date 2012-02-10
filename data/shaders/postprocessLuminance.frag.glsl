uniform sampler2DRect fboTex;
varying vec2 texcoord;
// downscale stage of making the bloom texture

void main(void)
{
	#define threshold 0.085
	const float delta = 0.001;//0.299,0.587,0.114
	vec3 col = vec3(texture2DRect(fboTex, texcoord.st));
	float lum = dot(col,vec3(0.2126, 0.7152, 0.0722));
	float switch_ = step(threshold,lum);// use to set to 0 if < threshold
	gl_FragColor = vec4(log(delta + lum));
	gl_FragColor.g = switch_;
	gl_FragColor.b = switch_*gl_FragColor.b; 
}
