#include "ShipSpinner.h"
#include "Ship.h"
#include "Pi.h"
#include "Game.h"

using namespace UI;

namespace GameUI {

ShipSpinner::ShipSpinner(Context *context, const ShipFlavour &flavour) : Widget(context),
	m_flavour(flavour),
	m_spin(0)
{}

void ShipSpinner::Layout()
{
	m_model = LmrLookupModelByName(ShipType::types[m_flavour.type].lmrModelName.c_str());

	memset(&m_params, 0, sizeof(LmrObjParams));
	m_params.animationNamespace = "ShipAnimation";
	m_params.equipment = &m_equipment;
	m_flavour.ApplyTo(&m_params);
	m_params.animValues[Ship::ANIM_WHEEL_STATE] = 1.0;
	m_params.flightState = Ship::FLYING;

	Color lc(0.5f, 0.5f, 0.5f, 0.f);
	m_light.SetDiffuse(lc);
	m_light.SetAmbient(lc);
	m_light.SetSpecular(lc);
	m_light.SetPosition(vector3f(1.f, 1.f, 0.f));
	m_light.SetType(Graphics::Light::LIGHT_DIRECTIONAL);
}

void ShipSpinner::Draw()
{
	Uint32 now = SDL_GetTicks();

	m_params.time = double(now) * 0.001;

	Graphics::Renderer *r = GetContext()->GetRenderer();

	Graphics::Renderer::StateTicket ticket(r);

	r->SetPerspectiveProjection(45.f, 1.f, 1.f, 10000.f);
	r->SetTransform(matrix4x4f::Identity());

	r->SetDepthTest(true);
	r->ClearDepthBuffer();

	r->SetLights(1, &m_light);

	Point pos(GetAbsolutePosition());
	Point size(GetSize());

	r->SetViewport(pos.x, GetContext()->GetSize().y - pos.y - size.y, size.x, size.y);

	float x = .5*m_spin;
	float y = m_spin;

	matrix4x4f rot = matrix4x4f::RotateXMatrix(x);
	rot.RotateY(y);
	rot[14] = -1.5f * m_model->GetDrawClipRadius();

	m_model->Render(rot, &m_params);

	m_spin += float(SDL_GetTicks()-now) * 0.001;
}

}
