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
	if (fade){
		vec4 pos = gl_ModelViewMatrix * gl_Vertex;
		pos.y = pos.y/0.4; // reverse scaling to increase density to represent milkyway
		/*float */angle = dot(upDir,vec3(normalize(pos)));		
		float blend = clamp(angle-sunAngle-0.1,0.0,0.2)/0.2;
		brightness = mix(b, darklevel, blend);
		if (angle < 0.3){b=1.0;}
	}
	gl_FrontColor = vec4(gl_Color.rgb,gl_Color.a*b);
	
}

