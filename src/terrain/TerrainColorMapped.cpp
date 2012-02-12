#include "Terrain.h"
#include "TerrainNoise.h"

using namespace TerrainNoise;

template <>
const char *TerrainColorFractal<TerrainColorMapped>::GetColorFractalName() const { return "TerrainColorMapped"; }

template <>
TerrainColorFractal<TerrainColorMapped>::TerrainColorFractal(const SBody *body) : Terrain(body)
{
}

template <>
vector3d TerrainColorFractal<TerrainColorMapped>::GetColor(const vector3d &p, double height, const vector3d &norm)
{
	double latitude = -asin(p.y);
	if (p.y < -1.0) latitude = -0.5*M_PI;
	if (p.y > 1.0) latitude = 0.5*M_PI;
//	if (!isfinite(latitude)) {
//		// p.y is just n of asin domain [-1,1]
//		latitude = (p.y < 0 ? -0.5*M_PI : M_PI*0.5);
//	}
	double longitude = atan2(p.x, p.z);
	double px = (((m_colorMapSizeX-1) * (longitude + M_PI)) / (2*M_PI));
	double py = ((m_colorMapSizeY-1)*(latitude + 0.5*M_PI)) / M_PI;
	int ix = int(floor(px));
	int iy = int(floor(py));
	ix = Clamp(ix, 0, m_colorMapSizeX-1);
	iy = Clamp(iy, 0, m_colorMapSizeY-1);
	double dx = px-ix;
	double dy = py-iy;

	// p0,3 p1,3 p2,3 p3,3
	// p0,2 p1,2 p2,2 p3,2
	// p0,1 p1,1 p2,1 p3,1
	// p0,0 p1,0 p2,0 p3,0
	vector3d map[4][4];
	for (int x=-1; x<3; x++) {
		for (int y=-1; y<3; y++) {
			int m = m_colorMap[Clamp(iy+y, 0, m_heightMapSizeY-1)*m_heightMapSizeX + Clamp(ix+x, 0, m_heightMapSizeX-1)];
			// modified from the sdl docs
			/* Get Red component */
			int temp = m & m_fmt.Rmask;  /* Isolate red component */
			temp = temp >> m_fmt.Rshift; /* Shift it down to 8-bit */
			temp = temp << m_fmt.Rloss;  /* Expand to a full 8-bit number */
			map[x+1][y+1].x  = (double(temp))/255.0;
			static int i;
			if (i<200){printf("red: %i, col %f m %i\n",temp,map[x+1][y+1].x,m); i++;}
			/* Get Green component */
			temp = m & m_fmt.Gmask;  /* Isolate green component */
			temp = temp >> m_fmt.Gshift; /* Shift it down to 8-bit */
			temp = temp << m_fmt.Gloss;  /* Expand to a full 8-bit number */
			map[x+1][y+1].y = double(temp)/255.0;

			/* Get Blue component */
			temp = m & m_fmt.Bmask;  /* Isolate blue component */
			temp = temp >> m_fmt.Bshift; /* Shift it down to 8-bit */
			temp = temp << m_fmt.Bloss;  /* Expand to a full 8-bit number */
			map[x+1][y+1].z = double(temp)/255.0;
		}
	}

	vector3d c[4];
	for (int j=0; j<4; j++) {
		vector3d d0 = map[0][j] - map[1][j];
		vector3d d2 = map[2][j] - map[1][j];
		vector3d d3 = map[3][j] - map[1][j];
		vector3d a0 = map[1][j];
		vector3d a1 = -(1/3.0)*d0 + d2 - (1/6.0)*d3;
		vector3d a2 = 0.5*d0 + 0.5*d2;
		vector3d a3 = -(1/6.0)*d0 - 0.5*d2 + (1/6.0)*d3;
		c[j] = a0 + a1*dx + a2*dx*dx + a3*dx*dx*dx;
	}
	
	vector3d d0 = c[0] - c[1];
	vector3d d2 = c[2] - c[1];
	vector3d d3 = c[3] - c[1];
	vector3d a0 = c[1];
	vector3d a1 = -(1/3.0)*d0 + d2 - (1/6.0)*d3;
	vector3d a2 = 0.5*d0 + 0.5*d2;
	vector3d a3 = -(1/6.0)*d0 - 0.5*d2 + (1/6.0)*d3;
	vector3d v = vector3d(0.1,0.1,0.1) + a0 + a1*dy + a2*dy*dy + a3*dy*dy*dy;

	// v is the interpolated colour - map[0][0] is a map point
	return v;//map[0][0];
//------------------------------------------------------------
	double n = m_invMaxHeight*height/2;
	if (n <= 0) return m_darkrockColor[0];		
	const double flatness = pow(p.Dot(norm), 6.0);
	const vector3d color_cliffs = m_rockColor[0];
	double equatorial_desert = (2.0-m_icyness)*(-1.0+2.0*octavenoise(4, 0.05, 2.0, (n*2.0)*p)) *
		1.0*(2.0-m_icyness)*(1.0-p.y*p.y);
	//double equatorial_region = octavenoise(GetFracDef(0), 0.54, p) * p.y * p.x;
	//double equatorial_region_2 = ridged_octavenoise(GetFracDef(1), 0.58, p) * p.x * p.x;
	// Below is to do with variable colours for different heights, it gives a nice effect.
	// n is height.
	vector3d col;
	col = interpolate_color(equatorial_desert, m_rockColor[2], m_darkrockColor[4]);
	//col = interpolate_color(equatorial_region, col, m_darkrockColor[4]);
	//col = interpolate_color(equatorial_region_2, m_rockColor[1], col);
	if (n > 0.9) {
		n -= 0.9; n *= 10.0;
		col = interpolate_color(n, m_rockColor[5], col );
		col = interpolate_color(flatness, color_cliffs, col);
	}
	else if (n > 0.8) {
		n -= 0.8; n *= 10.0;
		col = interpolate_color(n, col, m_rockColor[5]);
		col = interpolate_color(flatness, color_cliffs, col);
	}
	else if (n > 0.7) {
		n -= 0.7; n *= 10.0;
		col = interpolate_color(n, m_rockColor[4], col);
		col = interpolate_color(flatness, color_cliffs, col);
	}
	else if (n > 0.6) {
		n -= 0.6; n *= 10.0;
		col = interpolate_color(n, m_rockColor[0], m_rockColor[4]);
		col = interpolate_color(flatness, color_cliffs, col);
	}
	else if (n > 0.5) {
		n -= 0.5; n *= 10.0;
		col = interpolate_color(n, col, m_rockColor[0]);
		col = interpolate_color(flatness, color_cliffs, col);
	}
	else if (n > 0.4) {
		n -= 0.4; n *= 10.0;
		col = interpolate_color(n, m_darkrockColor[3], col);
		col = interpolate_color(flatness, color_cliffs, col);
	}
	else if (n > 0.3) {
		n -= 0.3; n *= 10.0;
		col = interpolate_color(n, col, m_darkrockColor[3]);
		col = interpolate_color(flatness, color_cliffs, col);
	}
	else if (n > 0.2) {
		n -= 0.2; n *= 10.0;
		col = interpolate_color(n, m_rockColor[1], col);
		col = interpolate_color(flatness, color_cliffs, col);
	}
	else if (n > 0.1) {
		n -= 0.1; n *= 10.0;
		col = interpolate_color(n, col, m_rockColor[1]);
		col = interpolate_color(flatness, color_cliffs, col);
	}
	else {
		n *= 10.0;
		col = interpolate_color(n, m_darkrockColor[0], col);
		col = interpolate_color(flatness, color_cliffs, col);
	}
	return col;
}
	