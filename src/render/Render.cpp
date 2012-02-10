#include "Render.h"
#include "RenderTarget.h"
#include <stdexcept>
#include <sstream>
#include <iterator>

static GLuint boundArrayBufferObject = 0;
static GLuint boundElementArrayBufferObject = 0;

namespace Render {

static bool initted = false;

static bool hdrAvailable = false;
static bool hdrEnabled = false;

Shader *simpleShader;
Shader *planetRingsShader[4];

SHADER_CLASS_BEGIN(PostprocessShader)
	SHADER_UNIFORM_SAMPLER(fboTex)
SHADER_CLASS_END()

SHADER_CLASS_BEGIN(PostprocessComposeShader)
	SHADER_UNIFORM_SAMPLER(fboTex)
	SHADER_UNIFORM_SAMPLER(bloomTex)
	SHADER_UNIFORM_FLOAT(avgLum)
	SHADER_UNIFORM_FLOAT(middleGrey)
	SHADER_UNIFORM_VEC4(s1)
	SHADER_UNIFORM_VEC4(s2)
	SHADER_UNIFORM_VEC3(s3)
	SHADER_UNIFORM_VEC4(o1)
	SHADER_UNIFORM_VEC4(o2)
	SHADER_UNIFORM_VEC3(o3)
	SHADER_UNIFORM_FLOAT(min1)
	SHADER_UNIFORM_FLOAT(THRESHOLD)
	SHADER_UNIFORM_FLOAT(binwidth)
SHADER_CLASS_END()

SHADER_CLASS_BEGIN(PostprocessDownsampleShader)
	SHADER_UNIFORM_SAMPLER(fboTex)
	SHADER_UNIFORM_FLOAT(avgLum)
	SHADER_UNIFORM_FLOAT(middleGrey)
SHADER_CLASS_END()

PostprocessDownsampleShader *postprocessBloom1Downsample;
PostprocessShader *postprocessBloom2Downsample, *postprocessBloom3VBlur, *postprocessBloom4HBlur, *postprocessLuminance;
PostprocessComposeShader *postprocessCompose;

SHADER_CLASS_BEGIN(BillboardShader)
	SHADER_UNIFORM_SAMPLER(some_texture)
SHADER_CLASS_END()

BillboardShader *billboardShader;

int State::m_numLights = 1;
float State::m_znear = 10.0f;
float State::m_zfar = 1e6f;
float State::m_invLogZfarPlus1;
Shader *State::m_currentShader = 0;


// 2D rectangle texture for rtt
class RectangleTarget : public RenderTarget {
public:
	RectangleTarget(unsigned int w, unsigned int h) :
		RenderTarget(w, h, GL_TEXTURE_RECTANGLE, Texture::Format(GL_RGB16F_ARB, GL_RGB, GL_HALF_FLOAT_ARB))
	{ }
};


// 2D target with mipmaps for capturing scene luminance
class LuminanceTarget : public RenderTarget {
public:
	LuminanceTarget(unsigned int w, unsigned int h) :
		RenderTarget(w, h, GL_TEXTURE_2D, Texture::Format(GL_RGB16F, GL_RGB, GL_FLOAT), true)
	{ }

	void UpdateMipmaps() {
		glGenerateMipmapEXT(GL_TEXTURE_2D);
	}
};


// 2d rectangle target, used as base for fancier scene targets
class SceneTarget : public RectangleTarget {
protected:
	SceneTarget(unsigned int w, unsigned int h) :
		RectangleTarget(w, h)
	{ }
};


// 2d rectangle target with 24-bit depth buffer attachment
class StandardSceneTarget : public SceneTarget {
public:
	StandardSceneTarget(unsigned int w, unsigned int h) :
		SceneTarget(w, h)
	{
		glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, m_fbo);

		glGenRenderbuffersEXT(1, &m_depth);
		glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, m_depth);
		glRenderbufferStorageEXT(GL_RENDERBUFFER_EXT, GL_DEPTH_COMPONENT24, w, h);
		glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, 0);
		glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT, GL_RENDERBUFFER_EXT, m_depth);

		CheckCompleteness();

		glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
	}

	~StandardSceneTarget() {
		glDeleteRenderbuffersEXT(1, &m_depth);
	}

private:
	GLuint m_depth;
};


// 2d rectangle target with multisampled colour and 24-bit depth buffer attachment
class MultiSampledSceneTarget : public SceneTarget {
public:
	MultiSampledSceneTarget(unsigned int w, int h, int samples) :
		SceneTarget(w, h)
	{
		const Texture::Format format = GetFormat();

		// multisampled fbo
		glGenFramebuffersEXT(1, &m_msFbo);
		glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, m_msFbo);

		// ms color buffer
		glGenRenderbuffersEXT(1, &m_msColor);
		glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, m_msColor);
		glRenderbufferStorageMultisampleEXT(GL_RENDERBUFFER_EXT, samples, format.internalFormat, w, h);

		// ms depth buffer
		glGenRenderbuffersEXT(1, &m_msDepth);
		glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, m_msDepth);
		glRenderbufferStorageMultisampleEXT(GL_RENDERBUFFER_EXT, samples, GL_DEPTH_COMPONENT24, w, h);

		// attach
		glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_RENDERBUFFER_EXT, m_msColor);
		glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT,  GL_RENDERBUFFER_EXT, m_msDepth);

		CheckCompleteness();
	}

	~MultiSampledSceneTarget() {
		glDeleteRenderbuffersEXT(1, &m_msDepth);
		glDeleteRenderbuffersEXT(1, &m_msColor);
		glDeleteFramebuffersEXT(1, &m_msFbo);
	}

	virtual void BeginRTT() {
		//begin rendering to multisampled FBO
		glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, m_msFbo);
		glPushAttrib(GL_VIEWPORT_BIT);
		glViewport(0, 0, GetWidth(), GetHeight());
	}

	virtual void EndRTT() {
		//blit multisampled rendering to normal fbo
		glPopAttrib();
		glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);

		glBindFramebufferEXT(GL_READ_FRAMEBUFFER_EXT, m_msFbo);
		glBindFramebufferEXT(GL_DRAW_FRAMEBUFFER_EXT, m_fbo);

		//depth testing has already been done, so color is enough
		glBlitFramebufferEXT(0, 0, GetWidth(), GetHeight(), 0, 0, GetWidth(), GetHeight(), GL_COLOR_BUFFER_BIT, GL_NEAREST);

		glBindFramebufferEXT(GL_READ_FRAMEBUFFER_EXT, 0);
		glBindFramebufferEXT(GL_DRAW_FRAMEBUFFER_EXT, 0);
	}

private:
	GLuint m_msFbo;
	GLuint m_msColor;
	GLuint m_msDepth;
};


void BindArrayBuffer(GLuint bo)
{
	if (boundArrayBufferObject != bo) {
		glBindBufferARB(GL_ARRAY_BUFFER_ARB, bo);
		boundArrayBufferObject = bo;
	}
}

bool IsArrayBufferBound(GLuint bo)
{
	return boundArrayBufferObject == bo;
}

void BindElementArrayBuffer(GLuint bo)
{
	if (boundElementArrayBufferObject != bo) {
		glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, bo);
		boundElementArrayBufferObject = bo;
	}
}

bool IsElementArrayBufferBound(GLuint bo)
{
	return boundElementArrayBufferObject == bo;
}

void UnbindAllBuffers()
{
	BindElementArrayBuffer(0);
	BindArrayBuffer(0);
}

static struct postprocessBuffers_t {
	bool complete;
	RectangleTarget *halfSizeRT;
	LuminanceTarget *luminanceRT;
	RectangleTarget *bloom1RT;
	RectangleTarget *bloom2RT;
	SceneTarget     *sceneRT;
	int width, height;

	void CreateBuffers(int screen_width, int screen_height) {
		width = screen_width;
		height = screen_height;

		GLint msSamples = 0;
		glGetIntegerv(GL_SAMPLES, &msSamples);

		halfSizeRT  = new RectangleTarget(width>>1, height>>1);
		luminanceRT = new LuminanceTarget(128, 128);
		bloom1RT = new RectangleTarget(width>>2, height>>2);
		bloom2RT = new RectangleTarget(width>>2, height>>2);
		sceneRT = 0;
		if (msSamples > 1) {
			try {
				sceneRT = new MultiSampledSceneTarget(width, height, msSamples);
			} catch (RenderTarget::fbo_incomplete &ex) {
				if (ex.GetErrorCode() == GL_FRAMEBUFFER_UNSUPPORTED_EXT) {
					// try again without multisampling
					glDisable(GL_MULTISAMPLE);
					sceneRT = 0;
					msSamples = 1;
				} else {
					throw;
				}
			}
		}
		if (!sceneRT)
			sceneRT = new StandardSceneTarget(width, height);

		postprocessBloom1Downsample = new PostprocessDownsampleShader("postprocessBloom1Downsample", "#extension GL_ARB_texture_rectangle : enable\n");
		postprocessBloom2Downsample = new PostprocessShader("postprocessBloom2Downsample", "#extension GL_ARB_texture_rectangle : enable\n");
		postprocessBloom3VBlur = new PostprocessShader("postprocessBloom3VBlur");
		postprocessBloom4HBlur = new PostprocessShader("postprocessBloom4HBlur");
		postprocessCompose = new PostprocessComposeShader("postprocessCompose", "#extension GL_ARB_texture_rectangle : enable\n");
		postprocessLuminance = new PostprocessShader("postprocessLuminance", "#extension GL_ARB_texture_rectangle : enable\n");

		glError();
	}
	void DeleteBuffers() {
		delete halfSizeRT;
		delete luminanceRT;
		delete bloom1RT;
		delete bloom2RT;
		delete sceneRT;

		delete postprocessBloom1Downsample;
		delete postprocessBloom2Downsample;
		delete postprocessBloom3VBlur;
		delete postprocessBloom4HBlur;
		delete postprocessCompose;
		delete postprocessLuminance;
	}
	void DoPostprocess() {
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		glOrtho(0.0,1.0,0.0,1.0,-1.0,1.0);
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		glDisable(GL_DEPTH_TEST);
		glDisable(GL_BLEND);
		glDisable(GL_LIGHTING);

		// So, to do proper tone mapping of HDR to LDR we need to know the average luminance
		// of the scene. We do this by rendering the scene's luminance to a smaller texture,
		// generating mipmaps for it, and grabbing the luminance at the smallest mipmap level
		luminanceRT->BeginRTT();
		sceneRT->BindTexture();
		State::UseProgram(postprocessLuminance);
		postprocessLuminance->set_fboTex(0);
		glBegin(GL_TRIANGLE_STRIP);
			glTexCoord2f(0.0, 0.0);
			glVertex2f(0.0, 0.0);
			glTexCoord2f(float(width), 0.0);
			glVertex2f(1.0, 0.0);
			glTexCoord2f(0.0,float(height));
			glVertex2f(0.0, 1.0);
			glTexCoord2f(float(width), float(height));
			glVertex2f(1.0, 1.0);
		glEnd();
		luminanceRT->EndRTT();
		sceneRT->UnbindTexture();

		luminanceRT->BindTexture();
		luminanceRT->UpdateMipmaps();
		float avgLum[4];
		glGetTexImage(GL_TEXTURE_2D, 7, GL_RGB, GL_FLOAT, avgLum);
		static Sint64 ii = 0;ii++;
		double lumavg = avgLum[0];
		//avgLum[1] is the fraction of the scene not black space, avgLum[2] = (lum>threshold)?avgLum[0]:0
		avgLum[0]= float((avgLum[2])/avgLum[1]);
		
		//printf("%f -> ", avgLum[0]);
		avgLum[0] = std::max(float(exp(avgLum[0])), 0.03f);

		float lumim[128*128*3];
		glGetTexImage(GL_TEXTURE_2D, 0, GL_RGB, GL_FLOAT, lumim);

		double min=100000.0;double max = log(0.001); 
		for (int i = 0;i<(128*128);i++){
			lumim[i*3]-=float(log(0.001));double lum = lumim[i*3];max=(lum>max)?lum:max; min=(lum<min)?lum:min;
		}
#define NUMBINS 10
// threshold - lowest of 0.1 or 5% of average
#define THRESHOLD -5.0/*std::min(4.6052,avgLum[0]+log(0.08))*/
		if (max-min<0.00001) {max = min + 0.01;} //avoid div by 0
		double range = max-min;
		double binwidth = range/double(NUMBINS);
		int binsTarget[NUMBINS+1] ={0,0,0,0,0,0,0,0,0,0,0}; 
		int binsCurrent[NUMBINS+1] ={0,0,0,0,0,0,0,0,0,0,0};		
		static int binsOld[NUMBINS+1] = {0,0,0,0,0,0,0,0,0,0,0};
		static double minold;
		static double binwidthOld;
		
		
		// calculate min and binwidth values based on new (target) values and previous values
		/*min = 0.01*(min-minold)+min;
		minold = min;
		binwidth = 0.01*(binwidth-binwidthOld)+binwidthOld;
		binwidthOld = binwidth;
		range = binwidth*NUMBINS;//used for debugging printfs */

		for (int i = 0;i<(128*128);i++){
			double lum = lumim[i*3];
			if (lum<THRESHOLD) {binsTarget[0]++;}
			else {int c = Clamp(int(ceil((lum-min)/binwidth)),1,10);binsTarget[c]+=1;} //1 to NUMBINS
		}
		
		// set current value based on target and old value
		int total = 0;int idx = NUMBINS;while(idx>-1){binsCurrent[idx] = int(floor(1.0*double(-binsOld[idx]+binsTarget[idx]))+binsOld[idx]); binsOld[idx] = binsCurrent[idx];binsCurrent[idx] = binsTarget[idx];total+=binsCurrent[idx];idx--;printf("%i\n",idx);}

		double fraction=0.0;
		float scaling[NUMBINS+1];
		float offsets[NUMBINS+1];
		int numValues = total-binsCurrent[0]; numValues = (numValues>=1)?numValues:1;
		double logRange = log(255.0); //width of possible output values roughly 2 orders of magnitude
		double spaceReservedFraction = 0.04;//exp(THRESHOLD+log(0.01))/4;
		double pos = spaceReservedFraction; // small fraction is left to map values below threshold
		for (int i = 0;i<NUMBINS;i++){
			// set values to scaling and offsets such that:
			// ((loglum-log(0.001)-min)-binwidth*binnum_minus1)*scaling[binnum] + offset[binnum] (where loglum = log(0.001+lum))
			// maps the interval binwidth*binnum_minus1 to binwidth*binnum
			// to the corresponding fraction of the output range (e.g. bottom bin has 20% of values so will get mapped to bottom 20% of output.)
			fraction = double(binsCurrent[i+1])/double(numValues);
			scaling[i+1] = float((1.0-spaceReservedFraction)*logRange*fraction/binwidth);
			offsets[i+1] = float(pos);
			if (double(ii)/60.0>=1.0){printf("bin %i, fraction in bin %f, cumulative %f, range %f, binwidth %f\n",i+1,fraction,pos/logRange,binwidth);}
			pos += fraction*logRange*(1.0-spaceReservedFraction);
		}
		scaling[0] = float(spaceReservedFraction/(THRESHOLD - min));
		offsets[0] = 0.0;



		if (double(ii)/60.0>=1.0){
			for (int i = 0;i<(101);i++){
				double l = range*(double(i)/100.0)+min;//-log(0.001) in shaders
				
				int b = (l>THRESHOLD)?Clamp(int(ceil((l-min)/binwidth)),1,10):0;
				
				l = ((l-min)-binwidth*(double(b)-1.0))*scaling[b] + offsets[b];
				printf("lum %f,out %f, lum frac %f, out frac %f, bin %i, scaling %f, offset %f,min %f, output range %f\n",range*(double(i)/100.0)+min,l,(double(i)/100.0)+min-min,(l-spaceReservedFraction)/(logRange*(1.0-spaceReservedFraction)),b,scaling[b],offsets[b],min,logRange); }
				//for (int iii = 0;iii<=NUMBINS;iii++){
				//}
			
		}


		//printf("%f\n", avgLum[0]);
		// see reinhard algo
		//float midGrey = 1.03f - 2.0f/(2.0f+log10(avgLum[0] + 1.0f));
		float midGrey = 3.5+0.0*(-log(avgLum[0])+min+range/2.)/(range/2.0);   

		
		if (double(ii)/60.0 >= 1.0) { ii=0; printf("avglum %f, lum log %f, fraction not space %f\n",avgLum[0],lumavg,avgLum[1]);}


		glDisable(GL_TEXTURE_2D);
		halfSizeRT->BeginRTT();
		sceneRT->BindTexture();
		State::UseProgram(postprocessBloom1Downsample);
		postprocessBloom1Downsample->set_avgLum(avgLum[0]);
		postprocessBloom1Downsample->set_middleGrey(midGrey);
		postprocessBloom1Downsample->set_fboTex(0);
		glBegin(GL_TRIANGLE_STRIP);
			glVertex2f(0.0, 0.0);
			glVertex2f(1.0, 0.0);
			glVertex2f(0.0, 1.0);
			glVertex2f(1.0, 1.0);
		glEnd();
		halfSizeRT->EndRTT();

		bloom1RT->BeginRTT();
		State::UseProgram(postprocessBloom2Downsample);
		halfSizeRT->BindTexture();
		glBegin(GL_TRIANGLE_STRIP);
			glVertex2f(0.0, 0.0);
			glVertex2f(1.0, 0.0);
			glVertex2f(0.0, 1.0);
			glVertex2f(1.0, 1.0);
		glEnd();
		halfSizeRT->UnbindTexture();
		bloom1RT->EndRTT();
		
		bloom2RT->BeginRTT();
		bloom1RT->BindTexture();
		State::UseProgram(postprocessBloom3VBlur);
		postprocessBloom3VBlur->set_fboTex(0);
		glBegin(GL_TRIANGLE_STRIP);
			glVertex2f(0.0, 0.0);
			glVertex2f(1.0, 0.0);
			glVertex2f(0.0, 1.0);
			glVertex2f(1.0, 1.0);
		glEnd();
		bloom2RT->EndRTT();

		bloom1RT->BeginRTT();
		bloom2RT->BindTexture();
		State::UseProgram(postprocessBloom4HBlur);
		postprocessBloom4HBlur->set_fboTex(0);
		glBegin(GL_TRIANGLE_STRIP);
			glTexCoord2f(0.0, 0.0);
			glVertex2f(0.0, 0.0);
			glTexCoord2f(1.0, 0.0);
			glVertex2f(1.0, 0.0);
			glTexCoord2f(0.0,1.0);
			glVertex2f(0.0, 1.0);
			glTexCoord2f(1.0, 1.0);
			glVertex2f(1.0, 1.0);
		glEnd();
		bloom1RT->EndRTT();
		
		glViewport(0,0,width,height);
		glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
		sceneRT->BindTexture();
		glActiveTexture(GL_TEXTURE1);
		bloom1RT->BindTexture();
		State::UseProgram(postprocessCompose);
		postprocessCompose->set_s1(scaling[0],scaling[1],scaling[2],scaling[3]);
		postprocessCompose->set_s2(scaling[4],scaling[5],scaling[6],scaling[7]);
		postprocessCompose->set_s3(scaling[8],scaling[9],scaling[10]);
		postprocessCompose->set_fboTex(0);
		postprocessCompose->set_o1(offsets[0],offsets[1],offsets[2],offsets[3]);
		postprocessCompose->set_o2(offsets[4],offsets[5],offsets[6],offsets[7]);
		postprocessCompose->set_o3(offsets[8],offsets[9],offsets[10]);
		postprocessCompose->set_binwidth(float(binwidth));
		postprocessCompose->set_min1(float(min));
		postprocessCompose->set_THRESHOLD(float(THRESHOLD));
		
		postprocessCompose->set_bloomTex(1);
		postprocessCompose->set_avgLum(avgLum[0]);
		//printf("Mid grey %f\n", midGrey);
		postprocessCompose->set_middleGrey(midGrey);
		glBegin(GL_TRIANGLE_STRIP);
			glVertex2f(0.0, 0.0);
			glVertex2f(1.0, 0.0);
			glVertex2f(0.0, 1.0);
			glVertex2f(1.0, 1.0);
		glEnd();
		State::UseProgram(0);

		glEnable(GL_DEPTH_TEST);
		bloom1RT->UnbindTexture();
		glActiveTexture(GL_TEXTURE0);
		sceneRT->UnbindTexture();
		glError();
	}
} s_hdrBufs;

void Init(int screen_width, int screen_height)
{
	if (initted) return;

	PrintGLInfo();

	shadersAvailable = glewIsSupported("GL_VERSION_2_0");
	shadersEnabled = shadersAvailable;
	printf("GLSL shaders %s.\n", shadersEnabled ? "on" : "off");

	// Framebuffers for HDR
	hdrAvailable = glewIsSupported("GL_EXT_framebuffer_object GL_ARB_color_buffer_float GL_ARB_texture_rectangle");
	if (hdrAvailable) {
		try {
			s_hdrBufs.CreateBuffers(screen_width, screen_height);
		} catch (RenderTarget::fbo_incomplete &ex) {
			if (ex.GetErrorCode() == GL_FRAMEBUFFER_UNSUPPORTED_EXT) {
				fprintf(stderr, "HDR render targets unsupported: forcing HDR off\n");
			} else {
				fprintf(stderr, "HDR initialization error: %s\n", ex.what());
			}
			s_hdrBufs.DeleteBuffers();
			hdrAvailable = false;
			hdrEnabled = false;
			glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
		}
	}
	
	initted = true;

	if (shadersEnabled) {
		simpleShader = new Shader("simple");
		billboardShader = new BillboardShader("billboard");
		planetRingsShader[0] = new Shader("planetrings", "#define NUM_LIGHTS 1\n");
		planetRingsShader[1] = new Shader("planetrings", "#define NUM_LIGHTS 2\n");
		planetRingsShader[2] = new Shader("planetrings", "#define NUM_LIGHTS 3\n");
		planetRingsShader[3] = new Shader("planetrings", "#define NUM_LIGHTS 4\n");
	}
}

void Uninit()
{
	s_hdrBufs.DeleteBuffers();
	delete simpleShader;
	delete billboardShader;
	delete planetRingsShader[0];
	delete planetRingsShader[1];
	delete planetRingsShader[2];
	delete planetRingsShader[3];
	FreeLibs();
}

bool IsHDREnabled() { return shadersEnabled && hdrEnabled; }
bool IsHDRAvailable() { return hdrAvailable; }

void PrepareFrame()
{
	if (IsHDRAvailable()) {
		if (IsHDREnabled())
			s_hdrBufs.sceneRT->BeginRTT();
		else
			glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
	}
}

void PostProcess()
{
	if (IsHDREnabled()) {
		s_hdrBufs.sceneRT->EndRTT();
		s_hdrBufs.DoPostprocess();
	}
}

void SwapBuffers()
{
	SDL_GL_SwapBuffers();
}

bool AreShadersEnabled()
{
	return shadersEnabled;
}

void ToggleShaders()
{
	if (shadersAvailable) {
		shadersEnabled = (shadersEnabled ? false : true);
	}
	printf("GLSL shaders %s.\n", shadersEnabled ? "on" : "off");
}

void ToggleHDR()
{
	if (hdrAvailable)
		hdrEnabled = !hdrEnabled;
	printf("HDR lighting %s.\n", hdrEnabled ? "enabled" : "disabled");
}

void GetNearFarClipPlane(float &znear, float &zfar)
{
	if (shadersEnabled) {
		/* If vertex shaders are enabled then we have a lovely logarithmic
		 * z-buffer stretching out from 0.1mm to 10000km! */
		znear = 0.0001f;
		zfar = 10000000.0f;
	} else {
		/* Otherwise we have the usual hopelessly crap z-buffer */
		znear = 10.0f;
		zfar = 1000000.0f;
	}
}

/**
 * So if we are using the z-hack VPROG_POINTSPRITE then this still works.
 * Desired texture should already be bound on calling PutPointSprites()
 */
void PutPointSprites(int num, vector3f *v, float size, const float modulationCol[4], int stride)
{
	glEnable(GL_BLEND);
	glDisable(GL_LIGHTING);
	glDepthMask(GL_FALSE);

//	float quadratic[] =  { 0.0f, 0.0f, 0.00001f };
//	glPointParameterfv( GL_POINT_DISTANCE_ATTENUATION, quadratic );
//	glPointParameterf(GL_POINT_SIZE_MIN, 1.0 );
//	glPointParameterf(GL_POINT_SIZE_MAX, 10000.0 );
		
	glEnable(GL_TEXTURE_2D);
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	glTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glColor4fv(modulationCol);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE);	

	// XXX point sprite thing needs some work. remember to enable point
	// sprite shader in LmrModel.cpp
	if (AreShadersEnabled()) {
		// this is a bit dumb since it doesn't care how many lights
		// the scene has, and this is a constant...
		State::UseProgram(billboardShader);
		billboardShader->set_some_texture(0);
	}

//	/*if (Shader::IsVtxProgActive())*/ glEnable(GL_VERTEX_PROGRAM_POINT_SIZE_ARB);
	if (0) {//GLEW_ARB_point_sprite) {
		glTexEnvf(GL_POINT_SPRITE_ARB, GL_COORD_REPLACE_ARB, GL_TRUE);
		glEnable(GL_POINT_SPRITE_ARB);
		
		glPointSize(size);
		glEnableClientState(GL_VERTEX_ARRAY);
		glVertexPointer(3, GL_FLOAT, 0, v);
		glDrawArrays(GL_POINTS, 0, num);
		glDisableClientState(GL_VERTEX_ARRAY);
		glPointSize(1);

		glDisable(GL_POINT_SPRITE_ARB);
		glDisable(GL_TEXTURE_2D);
	
	} else {
		// quad billboards
		matrix4x4f rot;
		glGetFloatv(GL_MODELVIEW_MATRIX, &rot[0]);
		rot.ClearToRotOnly();
		rot = rot.InverseOf();

		const float sz = 0.5f*size;
		const vector3f rotv1 = rot * vector3f(sz, sz, 0.0f);
		const vector3f rotv2 = rot * vector3f(sz, -sz, 0.0f);
		const vector3f rotv3 = rot * vector3f(-sz, -sz, 0.0f);
		const vector3f rotv4 = rot * vector3f(-sz, sz, 0.0f);

		glBegin(GL_QUADS);
		for (int i=0; i<num; i++) {
			vector3f pos(*v);
			vector3f vert;

			vert = pos+rotv4;
			glTexCoord2f(0.0f,0.0f);
			glVertex3f(vert.x, vert.y, vert.z);
			
			vert = pos+rotv3;
			glTexCoord2f(0.0f,1.0f);
			glVertex3f(vert.x, vert.y, vert.z);
			
			vert = pos+rotv2;
			glTexCoord2f(1.0f,1.0f);
			glVertex3f(vert.x, vert.y, vert.z);
			
			vert = pos+rotv1;
			glTexCoord2f(1.0f,0.0f);
			glVertex3f(vert.x, vert.y, vert.z);
			
			v = reinterpret_cast<vector3f*>(reinterpret_cast<char*>(v)+stride);
		}
		glEnd();
	}
//	/*if (Shader::IsVtxProgActive())*/ glDisable(GL_VERTEX_PROGRAM_POINT_SIZE_ARB);

//	quadratic[0] = 1; quadratic[1] = 0;
//	glPointParameterfvARB( GL_POINT_DISTANCE_ATTENUATION_ARB, quadratic );

	glDisable(GL_TEXTURE_2D);
	glDepthMask(GL_TRUE);
	glEnable(GL_LIGHTING);
	glDisable(GL_BLEND);
}

bool State::UseProgram(Shader *shader)
{
	if (shadersEnabled) {
		if (shader) {
			if (m_currentShader != shader) {
				m_currentShader = shader;
				glUseProgram(shader->GetProgram());
				shader->set_invLogZfarPlus1(m_invLogZfarPlus1);
				return true;
			} else {
				return false;
			}
		} else {
			m_currentShader = 0;
			glUseProgram(0);
			return true;
		}
	} else {
		return false;
	}
}

void PrintGLInfo() {
	std::string fname = GetPiUserDir() + "opengl.txt";
	FILE *f = fopen(fname.c_str(), "w");
	if (!f) return;

	std::ostringstream ss;
	ss << "OpenGL version " << glGetString(GL_VERSION);
	ss << ", running on " << glGetString(GL_VENDOR);
	ss << " " << glGetString(GL_RENDERER) << std::endl;

	ss << "Available extensions:" << std::endl;
	GLint numext = 0;
	glGetIntegerv(GL_NUM_EXTENSIONS, &numext);
	if (glewIsSupported("GL_VERSION_3_0")) {
		for (int i = 0; i < numext; ++i) {
			ss << "  " << glGetStringi(GL_EXTENSIONS, i) << std::endl;
		}
	}
	else {
		ss << "  ";
		std::istringstream ext(reinterpret_cast<const char *>(glGetString(GL_EXTENSIONS)));
		std::copy(
			std::istream_iterator<std::string>(ext),
			std::istream_iterator<std::string>(),
			std::ostream_iterator<std::string>(ss, "\n  "));
	}

	fprintf(f, "%s", ss.str().c_str());
	fclose(f);
	printf("OpenGL system information saved to %s\n", fname.c_str());
}

} /* namespace Render */
