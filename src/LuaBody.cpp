// Copyright © 2008-2012 Pioneer Developers. See AUTHORS.txt for details
// Licensed under the terms of the GPL v3. See licenses/GPL-3.txt

#include "LuaBody.h"
#include "LuaSystemPath.h"
#include "LuaUtils.h"
#include "LuaConstants.h"
#include "Body.h"
#include "galaxy/StarSystem.h"
#include "Frame.h"

/*
 * Class: Body
 *
 * Class represents a physical body.
 *
 * These objects only exist for the bodies of the system that the player is
 * currently in. If you need to retain a reference to a body outside of the
 * current system, look at <SystemBody>, <SystemPath> and the discussion of
 * <IsDynamic>.
 */

/*
 * Attribute: label
 *
 * The label for the body. This is what is displayed in the HUD and usually
 * matches the name of the planet, space station, etc if appropriate.
 *
 * Availability:
 *
 *   alpha 10
 *
 * Status:
 *
 *   stable
 */
static int l_body_attr_label(lua_State *l)
{
	Body *b = LuaBody::GetFromLua(1);
	lua_pushstring(l, b->GetLabel().c_str());
	return 1;
}

/*
 * Attribute: seed
 *
 * The random seed used to generate this <Body>. This is guaranteed to be the
 * same for this body across runs of the same build of the game, and should be
 * used to seed a <Rand> object when you want to ensure the same random
 * numbers come out each time.
 *
 * Availability:
 *
 *   alpha 10
 *
 * Status:
 *
 *   stable
 */
static int l_body_attr_seed(lua_State *l)
{
	Body *b = LuaBody::GetFromLua(1);

	const SystemBody *sbody = b->GetSystemBody();
	assert(sbody);

	lua_pushinteger(l, sbody->seed);
	return 1;
}

/*
 * Attribute: path
 *
 * The <SystemPath> that points to this body.
 *
 * If the body is a dynamic body it has no persistent path data, and its
 * <path> value will be nil.
 *
 * Availability:
 *
 *   alpha 10
 *
 * Status:
 *
 *   stable
 */
static int l_body_attr_path(lua_State *l)
{
	Body *b = LuaBody::GetFromLua(1);

	const SystemBody *sbody = b->GetSystemBody();
	if (!sbody) {
		lua_pushnil(l);
		return 1;
	}

	SystemPath path = sbody->path;
	LuaSystemPath::PushToLua(&path);

	return 1;
}

/*
 * Attribute: type
 *
 * The type of the body, as a <Constants.BodyType> constant.
 *
 * Only valid for non-dynamic <Bodies>. For dynamic bodies <type> will be nil.
 *
 * Availability:
 *
 *  alpha 10
 *
 * Status:
 *
 *  stable
 */
static int l_body_attr_type(lua_State *l)
{
	Body *b = LuaBody::GetFromLua(1);
	const SystemBody *sbody = b->GetSystemBody();
	if (!sbody) {
		lua_pushnil(l);
		return 1;
	}

	lua_pushstring(l, LuaConstants::GetConstantString(l, "BodyType", sbody->type));
	return 1;
}

/*
 * Attribute: superType
 *
 * The supertype of the body, as a <Constants.BodySuperType> constant
 *
 * Only valid for non-dynamic <Bodies>. For dynamic bodies <superType> will be nil.
 *
 * Availability:
 *
 *  alpha 10
 *
 * Status:
 *
 *  stable
 */
static int l_body_attr_super_type(lua_State *l)
{
	Body *b = LuaBody::GetFromLua(1);
	const SystemBody *sbody = b->GetSystemBody();
	if (!sbody) {
		lua_pushnil(l);
		return 1;
	}

	lua_pushstring(l, LuaConstants::GetConstantString(l, "BodySuperType", sbody->GetSuperType()));
	return 1;
}

/*
 * Attribute: frameBody
 *
 * The non-dynamic body attached to the frame this dynamic body is in.
 *
 * Only valid for dynamic <Bodies>. For non-dynamic bodies <frameBody> will be
 * nil.
 *
 * <frameBody> can also be nil if this dynamic body is in a frame with no
 * non-dynamic body. This most commonly occurs when the player is in
 * hyperspace.
 *
 * Availability:
 *
 *   alpha 12
 *
 * Status:
 *
 *   experimental
 */
static int l_body_attr_frame_body(lua_State *l)
{
	Body *b = LuaBody::GetFromLua(1);
	if (!b->IsType(Object::DYNAMICBODY)) {
		lua_pushnil(l);
		return 1;
	}

	Frame *f = b->GetFrame();
	LuaBody::PushToLua(f->GetBodyFor());
	return 1;
}

/*
 * Attribute: frameRotating
 *
 * Whether the frame this dynamic body is in is a rotating frame.
 *
 * Only valid for dynamic <Bodies>. For non-dynamic bodies <frameRotating>
 * will be nil.
 *
 * Availability:
 *
 *   alpha 12
 *
 * Status:
 *
 *   experimental
 */
static int l_body_attr_frame_rotating(lua_State *l)
{
	Body *b = LuaBody::GetFromLua(1);
	if (!b->IsType(Object::DYNAMICBODY)) {
		lua_pushnil(l);
		return 1;
	}

	Frame *f = b->GetFrame();
	lua_pushboolean(l, f->IsRotatingFrame());
	return 1;
}

/*
 * Method: IsDynamic
 *
 * Determine if the body is a dynamic body
 *
 * > isdynamic = body:IsDynamic()
 *
 * A dynamic body is one that is not part of the generated system. Currently
 * <Ships> and <CargoBodies> are dynamic bodies. <Stars>, <Planets> and
 * <SpaceStations> are not.
 *
 * Being a dynamic body generally means that there is no way to reference the
 * body outside of the context of the current system. A planet, for example,
 * can always be referenced by its <SystemPath> (available via <Body.path>),
 * even from outside the system. A <Ship> however can not be referenced in
 * this way. If a script needs to retain information about a ship that is no
 * longer in the <Player's> current system it must manage this itself.
 *
 * The above list of static/dynamic bodies may change in the future. Scripts
 * should use this method to determine the difference rather than checking
 * types directly.
 *
 * Availability:
 *
 *   alpha 10
 *
 * Status:
 *
 *   stable
 */
static int l_body_is_dynamic(lua_State *l)
{
	Body *b = LuaBody::GetFromLua(1);
	lua_pushboolean(l, b->IsType(Object::DYNAMICBODY));
	return 1;
}

/*
 * Method: DistanceTo
 *
 * Calculate the distance between two bodies
 *
 * > dist = body:DistanceTo(otherbody)
 *
 * Parameters:
 *
 *   otherbody - the body to calculate the distance to
 *
 * Returns:
 *
 *   dist - distance between the two bodies in meters
 *
 * Availability:
 *
 *   alpha 10
 *
 * Status:
 *
 *   stable
 */
static int l_body_distance_to(lua_State *l)
{
	Body *b1 = LuaBody::GetFromLua(1);
	Body *b2 = LuaBody::GetFromLua(2);
	if (!b1->IsInSpace())
		return luaL_error(l, "Body:DistanceTo() arg #1 is not in space (probably a ship in hyperspace)");
	if (!b2->IsInSpace())
		return luaL_error(l, "Body:DistanceTo() arg #2 is not in space (probably a ship in hyperspace)");
	lua_pushnumber(l, b1->GetPositionRelTo(b2).Length());
	return 1;
}

/*
 * Method: IsAnAstronomicalBody
 *
 * Determine if the body is an astronomical body
 * like a planet or a star and not like a space station or a grav point.
 *
 * > isAnAstronomicalBody = body:IsAnAstronomicalBody()
 *
 * Parameters:
 *            body - a body of any type.
 *
 * Availability:
 *
 *   alpha 28
 *
 * Status:
 *
 *   Experimental
 */

static int l_body_is_an_astronomical_body(lua_State *l)
{
	Body *b = LuaBody::GetFromLua(1);
	bool isAstronomicalBody = false;
	if (b->IsType(Object::PLANET)) {
		isAstronomicalBody = !((b->GetSystemBody()->GetSuperType() == SystemBody::SUPERTYPE_NONE)
	                          ||(b->GetSystemBody()->GetSuperType() == SystemBody::SUPERTYPE_STARPORT));
	}
	lua_pushnumber(l, isAstronomicalBody);
	return 1;
}

/*
 * Method: change_in_speed_required_to_escape_gravity_well
 *
 * Calculate the change in speed required to give a dynamic body 
 * like a ship enough kinetic energy to escape the gravitational well of a 
 * gravitational body like a planet.
 *
 * Note: this method will return a negative value if the body already has more than enough kinetic energy.
 *
 * > changeInSpeedRequiredToEscapeGravityWell = 
 *             orbitingBody:ChangeInSpeedRequiredToEscapeGravityWellOf(planetBody)
 *
 * Parameters:
 *
 *   planetBody - The body belonging to the planet whose gravitational pull
 *                is experienced by the orbiting body.
 *
 *   orbitingBody - The body experiencing the planets gravity 
 *                  (a body can be said to be an orbiting body even if it 
 *                  will escape or crash into the planet).
 *                  This body must not be on orbit rails like a moon or a space station.
 *                  It must be a dynamic body like a ship, a missile etc. See the comments for body:IsDynamic for details.
 *
 * Returns:
 *
 *   changeInSpeedRequiredToEscapeGravityWell - The difference between current speed and that required to escape the well
 *                                          in meters/s.
 *
 * Availability:
 *
 *   alpha 28
 *
 * Status:
 *
 *   Experimental
 */

// XXX this gives the escape velocity to required to reach a point at infinity. 
//     gravitational fields don't extend beyond several radii so perhaps we need to state the velocity to reach edge of the rotating frame.
// To do: the derivation to make reasoning followable throws up some useful intermedioate quantities (velocity, escape energies etc.). these may be worth exposing too.
//        compilers should be capable of doing the cancellations and combining the derivation into one expression

static int l_change_in_speed_required_to_escape_gravity_well(lua_State *l)
{
	Body *b = LuaBody::GetFromLua(1);
	Body *planet = LuaBody::GetFromLua(2);
	
	// check the orbiting body is a dynamic body
	// and not a body on orbit rails like a planet
	if (b->IsType(Object::DYNAMICBODY)) {
		// the body near the planet (usually a ship, but can be a missile etc.)
		DynamicBody *orbitingBody = static_cast<DynamicBody *>(b);

		// distance between planet center and body in meters
		const double distance = orbitingBody->GetPositionRelTo(planet).Length();
		// mass of ship in kg
		const double massOrbitingBody = orbitingBody->GetMass();

		// the mass of the planet in kg
		const double massPlanet = planet->GetMass();

		// velocity relative to planet
		const vector3d velRelToPlanet = orbitingBody->GetVelocityRelTo(planet);

		// speed of orbiting body relative to the planet in m/s
		const double speedRelToPlanet = velRelToPlanet.Length();

		// kinetic energy of the orbiting body by virtue of its motion relative to the planet
		// standard KE = (1/2) * m*v^2 formula. units of Joules
		const double kineticEnergyOrbitingBody = 0.5*massOrbitingBody*speedRelToPlanet*speedRelToPlanet;

		// This is the energy a body at infinity falling to this level due to gravitational attraction will have gained 
		// In other words, this is the energy required to escape the gravitational field of the planet.
		// Derivation:
		// it is derived by calculating the work done on the body by gravity in moving from infinity to 
		// a distance away from the planet center.
		// W = integral[F dr] (r=d) - integral[F dr] (r=0)
		// F = -G*Mp*Mb/r^2 , integral[F dr] = G*Mp*Mb/r
		// W = G*Mp*Mb/(d) - G*Mp*Mb/(+inf) = G*Mp*Mb/d - 0
		// W = G*Mp*Mb/d . Units of Joules.
		const double escapeEnergyOrbitingBody = (G*massOrbitingBody*massPlanet/distance);

		// the speed needed to escape the gravitational well in m/s (also known as escape velocity)
		// v = sqrt(2*energy/m) (c.f. energy = (1/2)*mv^2)
		const double escapeSpeed = sqrt(2.0*escapeEnergyOrbitingBody/massOrbitingBody);

		// change in speed relative to the planet required to escape from its gravity in meters per second
		// note this can be negative if 
		const double changeInSpeedNeededToEscape = escapeSpeed-speedRelToPlanet;

		if (!planet->IsInSpace())
			return luaL_error(l, "Body:ChangeInSpeedToEscapeGravityWellOf() arg #1 is not in space (probably a ship in hyperspace)");
		if (!orbitingBody->IsInSpace())
			return luaL_error(l, "Body:ChangeInSpeedToEscapeGravityWellOf() arg #2 is not in space (probably a ship in hyperspace)");
		lua_pushnumber(l, changeInSpeedNeededToEscape);
		return 1;
	} else {
		return luaL_error(l, "Body:ChangeInSpeedRequiredToEscapeGravityWellOf() arg #1 is not a body which can change speed like a ship or missile (i.e. not a dynamic body)");
	}
}

template <> const char *LuaObject<Body>::s_type = "Body";

template <> void LuaObject<Body>::RegisterClass()
{
	static luaL_Reg l_methods[] = {
		{ "IsDynamic",  l_body_is_dynamic  },
		{ "DistanceTo", l_body_distance_to },
		{ "BodyIsAnAstronomicalBody", l_body_is_an_astronomical_body },
		{ "ChangeInSpeedRequiredToEscapeGravityWellOf", l_change_in_speed_required_to_escape_gravity_well },
		{ 0, 0 }
	};

	static luaL_Reg l_attrs[] = {
		{ "label",         l_body_attr_label          },
		{ "seed",          l_body_attr_seed           },
		{ "path",          l_body_attr_path           },
		{ "type",          l_body_attr_type           },
		{ "superType",     l_body_attr_super_type     },
		{ "frameBody",     l_body_attr_frame_body     },
		{ "frameRotating", l_body_attr_frame_rotating },
		{ 0, 0 }
	};

	LuaObjectBase::CreateClass(s_type, NULL, l_methods, l_attrs, NULL);
}
