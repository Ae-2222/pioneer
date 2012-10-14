#ifndef _GAMEUI_SHIPSPINNER_H
#define _GAMEUI_SHIPSPINNER_H

#include "ui/Context.h"
#include "ShipFlavour.h"
#include "graphics/Light.h"
#include "LmrModel.h"
#include "EquipSet.h"

namespace GameUI {

class ShipSpinner : public UI::Widget {
public:
	ShipSpinner(UI::Context *context, const ShipFlavour &flavour);

	virtual UI::Point PreferredSize() { return UI::Point(INT_MAX); }
	virtual void Layout();
	virtual void Draw();

private:
	ShipFlavour m_flavour;

	float m_rotX, m_rotY;

	LmrModel *m_model;
	LmrObjParams m_params;
	// XXX m_equipment is currently not hooked up to anything,
	// it's just used to pass equipment parameters to the displayed model
	EquipSet m_equipment;
	//illumination in ship view
	Graphics::Light m_light;
};

}

#endif
