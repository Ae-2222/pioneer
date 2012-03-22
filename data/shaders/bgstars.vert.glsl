uniform float brightness;
uniform bool fade;
uniform vec3 upDir;
uniform float sunAngle;
uniform float darklevel;

void main(void)
{
#ifdef ZHACK
	gl_Position = logarithmicTransform();
#else
	gl_Position = 
#endif
	gl_PointSize = 1.0 + pow(gl_Color.r,3.0);

	float b = brightness;
	float angle=1.0;
	vec4 col = gl_Color;
	if (fade){
		vec4 pos = gl_ModelViewMatrix * gl_Vertex;
		pos.y = pos.y/0.4; // reverse scaling to increase density to represent milkyway
		angle = dot(upDir,normalize(vec3(pos)));		
		float blend = clamp(angle-sunAngle-0.1,0.0,0.2)/0.2;
		brightness = mix(b, darklevel, blend);

		// debug
		// set stars above angle to blue and below to red.
		if (angle < 0.4){b=1.0;col.r = 1.0;}
		else {b=1.0;col.b = 1.0;}

	}
	gl_FrontColor = vec4(col.rgb/*gl_Color.rgb*/,gl_Color.a*b);
	
}

