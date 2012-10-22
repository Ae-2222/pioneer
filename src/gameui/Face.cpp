#include "Face.h"
#include "FileSystem.h"
#include "SDLWrappers.h"
#include "graphics/TextureBuilder.h"

using namespace UI;

namespace GameUI {

static const Uint32 FACE_WIDTH = 295;
static const Uint32 FACE_HEIGHT = 285;

// XXX these shouldn't really be hardcoded. it'd be much nicer to poke through
// the facegen/ dir and figure out what we've got available. that or some
// config file
static const Uint32 MAX_HEAD = 20;
static const Uint32 MAX_EYES = 20;
static const Uint32 MAX_NOSE = 20;
static const Uint32 MAX_MOUTH = 20;
static const Uint32 MAX_HAIR = 20;

static const Uint32 MAX_CLOTHES = 20;
static const Uint32 MAX_ARMOUR = 3;
static const Uint32 MAX_ACCESSORIES = 16;

static const Uint32 MAX_BACKGROUND = 18;

static void _blit_image(SDL_Surface *s, const char *filename, int xoff, int yoff)
{
	SDLSurfacePtr is = LoadSurfaceFromFile(filename);
	// XXX what should this do if the image couldn't be loaded?
	if (! is) { return; }

	SDL_Rect destrec = { 0, 0, 0, 0 };
	destrec.x = ((FACE_WIDTH-is->w-1)/2)+xoff;
	destrec.y = yoff;
	SDL_BlitSurface(is.Get(), NULL, s, &destrec);
}

static void _blit_image(const SDLSurfacePtr &s, const char *filename, int xoff, int yoff)
{
	_blit_image(s.Get(), filename, xoff, yoff);
}

Face::Face(Context *context, Uint32 flags, Uint32 seed) : Widget(context)
{
	if (!seed) seed = time(0);
	MTRand rand(seed);

	m_flags = flags;
	m_seed = seed;

	int race = rand.Int32(0,2);

	int gender;
	switch (flags & GENDER_MASK) {
		case MALE:
			gender = 0;
			break;
		case FEMALE:
			gender = 1;
			break;
		case RAND:
		default:
			gender = rand.Int32(0,1);
			break;
	}

	int head  = rand.Int32(0,MAX_HEAD);
	int eyes  = rand.Int32(0,MAX_EYES);
	int nose  = rand.Int32(0,MAX_NOSE);
	int mouth = rand.Int32(0,MAX_MOUTH);
	int hair  = rand.Int32(0,MAX_HAIR);

	int clothes = rand.Int32(0,MAX_CLOTHES);

	int armour = rand.Int32(0,MAX_ARMOUR);

	int accessories = rand.Int32(0,MAX_ACCESSORIES);

	int background = rand.Int32(0,MAX_BACKGROUND);

	char filename[1024];

	SDLSurfacePtr faceim = SDLSurfacePtr::WrapNew(SDL_CreateRGBSurface(SDL_SWSURFACE, FACE_WIDTH, FACE_HEIGHT, 24, 0xff, 0xff00, 0xff0000, 0));

	snprintf(filename, sizeof(filename), "facegen/backgrounds/background_%d.png", background);
	_blit_image(faceim, filename, 0, 0);

	snprintf(filename, sizeof(filename), "facegen/race_%d/head/head_%d_%d.png", race, gender, head);
	_blit_image(faceim, filename, 0, 0);

	if (!(flags & ARMOUR)) {
		snprintf(filename, sizeof(filename), "facegen/clothes/cloth_%d_%d.png", gender, clothes);
		_blit_image(faceim, filename, 0, 135);
	}

	snprintf(filename, sizeof(filename), "facegen/race_%d/eyes/eyes_%d_%d.png", race, gender, eyes);
	_blit_image(faceim, filename, 0, 41);

	snprintf(filename, sizeof(filename), "facegen/race_%d/nose/nose_%d_%d.png", race, gender, nose);
	_blit_image(faceim, filename, 1, 89);

	snprintf(filename, sizeof(filename), "facegen/race_%d/mouth/mouth_%d_%d.png", race, gender, mouth);
	_blit_image(faceim, filename, 0, 155);

	if (!(flags & ARMOUR)) {
		snprintf(filename, sizeof(filename), "facegen/accessories/acc_%d.png", accessories);
		if (rand.Int32(0,1)>0)	_blit_image(faceim, filename, 0, 0);

		snprintf(filename, sizeof(filename), "facegen/race_%d/hair/hair_%d_%d.png", race, gender, hair);
		_blit_image(faceim, filename, 0, 0);
	}
	else {
		snprintf(filename, sizeof(filename), "facegen/clothes/armour_%d.png", armour);
		_blit_image(faceim, filename, 0, 0);
	}

	m_quad.Reset(new Gui::TexturedQuad(Graphics::TextureBuilder(faceim, Graphics::LINEAR_CLAMP, true, true).CreateTexture(GetContext()->GetRenderer())));
}

void Face::Layout()
{
	Point size(GetSize());
	Point activeArea(std::min(size.x, size.y));
	Point activeOffset(std::max(0, (size.x-activeArea.x)/2), std::max(0, (size.y-activeArea.y)/2));
	SetActiveArea(activeArea, activeOffset);
}

void Face::Draw()
{
    const Point &offset = GetActiveOffset();
    const Point &area = GetActiveArea();

    Graphics::Renderer *r = GetContext()->GetRenderer();
    m_quad->Draw(r, vector2f(offset.x, offset.y), vector2f(area.x, area.y));
}

}
