uniform vec4 atmosColor;
// to keep distances sane we do a nearer, smaller scam. this is how many times
// smaller the oceansphere has been made
uniform float oceansphereScale;
uniform float oceansphereScaledRadius;
uniform float oceansphereAtmosTopRad;
uniform vec3 oceansphereCenter;
uniform float oceansphereAtmosFogDensity;
uniform float oceansphereAtmosInvScaleHeight;

uniform Scene scene;

varying vec3 varyingEyepos;
varying vec3 varyingNormal;

void main(void)
{
	vec3 eyepos = varyingEyepos;
	vec3 eyenorm = normalize(eyepos);
	vec3 tnorm = normalize(varyingNormal);
	vec4 diff = vec4(0.0);
	vec4 vertexColor = vec4(1.0, 0.0, 0.0, 1.0);

#if (NUM_LIGHTS > 0)
	for (int i=0; i<NUM_LIGHTS; ++i) {
		float nDotVP = max(0.0, dot(tnorm, normalize(vec3(gl_LightSource[i].position))));
		diff += gl_LightSource[i].diffuse * nDotVP;
	}

#ifdef ATMOSPHERE
	// when does the eye ray intersect atmosphere
	float atmosStart = findSphereEyeRayEntryDistance(oceansphereCenter, eyepos, oceansphereScaledRadius * oceansphereAtmosTopRad);

	float fogFactor;
	{
		float atmosDist = oceansphereScale * (length(eyepos) - atmosStart);
		float ldprod;
		// a&b scaled so length of 1.0 means planet surface.
		vec3 a = (atmosStart * eyenorm - oceansphereCenter) / oceansphereScaledRadius;
		vec3 b = (eyepos - oceansphereCenter) / oceansphereScaledRadius;
		ldprod = AtmosLengthDensityProduct(a, b, atmosColor.w*oceansphereAtmosFogDensity, atmosDist, oceansphereAtmosInvScaleHeight);
		fogFactor = 1.0 / exp(ldprod);
	}

	vec4 atmosDiffuse = vec4(0.0);
	{
		vec3 surfaceNorm = normalize(atmosStart*eyenorm - oceansphereCenter);
		for (int i=0; i<NUM_LIGHTS; ++i) {
			atmosDiffuse += gl_LightSource[i].diffuse * max(0.0, dot(surfaceNorm, normalize(vec3(gl_LightSource[i].position))));
		}
	}
	atmosDiffuse.a = 1.0;

	gl_FragColor =
		gl_FrontMaterial.emission +
		fogFactor *
		((scene.ambient * vertexColor) +
		(diff * vertexColor)) +
		(1.0-fogFactor)*(atmosDiffuse*atmosColor);
#else // atmosphere-less planetoids and dim stars
	gl_FragColor =
		gl_FrontMaterial.emission +
		(scene.ambient * vertexColor) +
		(diff * vertexColor);
#endif //ATMOSPHERE

#else // NUM_LIGHTS > 0 -- unlit rendering - stars
	//emission is used to boost colour of stars, which is a bit odd
	gl_FragColor = gl_FrontMaterial.emission + vertexColor;
#endif
	SetFragDepth();
}
