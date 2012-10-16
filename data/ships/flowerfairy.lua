-- Copyright © 2008-2012 Pioneer Developers. See AUTHORS.txt for details
-- Licensed under the terms of CC-BY-SA 3.0. See licenses/CC-BY-SA-3.0.txt

define_ship {
	name='Flowerfairy Heavy Trader',
	model='flowerfairy',
	forward_thrust = 60e6,
	reverse_thrust = 20e6,
	up_thrust = 20e6,
	down_thrust = 10e6,
	left_thrust = 10e6,
	right_thrust = 10e6,
	angular_thrust = 220e6,
	cockpit_front = v(0,4,-35),
	cockpit_rear = v(0,11,-26),
	front_camera = v(0,-3.8,-46),
	rear_camera = v(0,1,34),
	left_camera = v(-14,0,0),
	right_camera = v(14,0,0),
	top_camera = v(0,11,0),
	bottom_camera = v(0,-11,0),
	gun_mounts =
	{
		{ v(0,-0.5,0), v(0,0,-1), 5, 'HORIZONTAL' },
		{ v(0,0,0), v(0,0,1), 5, 'HORIZONTAL' },
	},
	max_atmoshield = 0,
	max_cargo = 500,
	max_laser = 2,
	max_missile = 4,
	max_cargoscoop = 0,
	capacity = 500,
	hull_mass = 350,
	fuel_tank_mass = 150,
	thruster_fuel_use = 0.0002,
	price = 550000,
	hyperdrive_class = 6,
}