uniform sampler2DRect fboTex;
uniform sampler2DRect bloomTex;
uniform float avgLum;
uniform float middleGrey;
uniform float min1;
uniform float THRESHOLD;
uniform float binwidth;
uniform vec4 s1;
uniform vec4 o1;
uniform vec4 s2;
uniform vec4 o2;
uniform vec3 s3;
uniform vec3 o3;
#define NUMBINS 10

void main(void)
{

	vec3 col = vec3(texture2DRect(fboTex, vec2(gl_FragCoord.x, gl_FragCoord.y)));
	//col += 0.0 * vec3(texture2DRect(bloomTex, vec2(gl_FragCoord.x*0.25, gl_FragCoord.y*0.25)));
	

	float offsets[NUMBINS+1] = float[](o1.x,o1.y,o1.z,o1.w,  o2.x,o2.y,o2.z,o2.w,  o3.x,o3.y,o3.z);
	float scaling[NUMBINS+1] = float[](s1.x,s1.y,s1.z,s1.w,  s2.x,s2.y,s2.z,s2.w,  s3.x,s3.y,s3.z);
	// This is the reinhard tonemapping algorithm, i think...
	float X,Y,Z,x,y;
	X = dot(vec3(0.4124, 0.3576, 0.1805), col);
	Y = dot(vec3(0.2126, 0.7152, 0.0722), col);
	Z = dot(vec3(0.0193, 0.1192, 0.9505), col);
	x = X / (X+Y+Z);
	y = Y / (X+Y+Z);
	// Y is luminance. fiddle with it

	//vec3 colr = col;
	//float yw = Y;

	// calculate log luminance
	//Y = log(0.001+Y)-log(0.001); // log y, offset so lowest is 0.0

	// calculate index	
	float bb = (clamp(ceil((Y-min1)/binwidth),1.0,10.0));
	int b = int(step(THRESHOLD,Y)*bb);


	//Y = ((Y-min1)-binwidth*(float(b)-1.0))*scaling[b] + offsets[b];
	//Y = max(Y,0.0);


	Y = (Y)/6.0;
	//Y = clamp(Y,0.0,1.0);
	//if (o2.x>= 2.0) {Y=0.4;} else {Y = 0.0;}
	//Y = 0.1;
	//Y = exp(Y)/4.0;
	//Y = clamp(Y,0.0,1.0);
	//Y = Y / (1.0*(avgLum-log(0.001)));
	Y = Y * (middleGrey / avgLum)*1.0; // scale luminance 
	Y = Y / (1.0 + Y); // compress luminance
	

	// convert back to RGB
	X = x*(Y/y);
	Z = (1.0-x-y)*(Y/y);
	col.r = 3.2405*X - 1.5371*Y - 0.4985*Z;
	col.g = -0.9693*X + 1.8760*Y + 0.0416*Z;
	col.b = 0.0556*X - 0.2040*Y + 1.0572*Z;

	// goodnight rgb uncompression
	//const float alpha = 1.0;
	//col = pow((colr/yw),alpha)*Y;

	gl_FragColor = vec4(col.r, col.g, col.b, 1.0);
}
