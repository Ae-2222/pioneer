//http://stackoverflow.com/questions/4200224/random-noise-functions-for-glsl

float noise2dtrig(vec2 x){
    return abs(fract(sin(dot(x.xy ,vec2(12.9898,78.233))) * 43758.5453));
}


uniform float brightness;
uniform float time;
uniform bool twinkling;
uniform float effect;
uniform vec3 upDir;	
uniform float sunAngle;
uniform bool fade;
uniform float darklevel;

void main(void)
{
#ifdef ZHACK
	gl_Position = logarithmicTransform();
#else
	gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;
#endif

float starSize = pow(gl_Color.r,4.0);
	float b = 1.0;
	if (twinkling){
		
 		//create an id for each star
		float p = (dot(vec3(gl_Vertex),vec3((20.0/1000.0))));
		//p = permute(p);
		//input time and star id as coordinates of 2d noise space
		b = pow(0.4+0.6*noise2dtrig(vec2(p, time)),0.4);
		// for small stars reduce range of change
		b = mix(1.0,b,(clamp(starSize,0.00,0.2))*(1.0/0.2));
		// blend away from 1.0 with effect
		b = mix(1.0,b,effect);
	}

	vec4 col = vec4(0.0); col.a = 1.0;

	if (1||fade){
		vec4 pos = gl_ModelViewMatrix * gl_Vertex;
		//pos.y = pos.y/0.4; // reverse scaling to increase density to represent milkyway
		float angle = dot(upDir,normalize(vec3(pos)));    
		float blend = clamp(angle-sunAngle-0.1,0.0,0.2)/0.2;
		brightness = mix(b, darklevel, blend);

		// debug
		// set stars above angle to blue and below to red.
		if (angle < 0.4){b=1.0;col.r = 1.0;}
		else {b=1.0;col.b = 1.0;}
	}

	gl_PointSize = 1.0 + (b*2.5+0.8)*starSize; //b controls a portion of star size 
	gl_FrontColor = col;//vec4(gl_Color.rgb,gl_Color.a*gl_FrontMaterial.emission*b);
}

