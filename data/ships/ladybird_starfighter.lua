-- Copyright © 2008-2012 Pioneer Developers. See AUTHORS.txt for details
-- Licensed under the terms of CC-BY-SA 3.0. See licenses/CC-BY-SA-3.0.txt

define_ship {
	name='Ladybird Starfighter',
	model='ladybird',
	forward_thrust = 10e6,
	reverse_thrust = 4e6,
	up_thrust = 3e6,
	down_thrust = 3e6,
	left_thrust = 2e6,
	right_thrust = 2e6,
	angular_thrust = 16e6,
	cockpit_front = v(0,2,-10),
	cockpit_rear = v(0,4,-8),
	front_camera = v(0,-3.5,-16),
	rear_camera = v(0,1,8.5),
	left_camera = v(-24,-3.5,7),
	right_camera = v(24,-3.5,7),
	top_camera = v(0,4,0),
	bottom_camera = v(0,-4.5,0),
	gun_mounts =
	{
		{ v(0,-0.5,0), v(0,0,-1), 5, 'HORIZONTAL' },
		{ v(0,0,0), v(0,0,1), 5, 'HORIZONTAL' },
	},
	max_cargo = 60,
	max_missile = 2,
	max_laser = 2,
	max_cargoscoop = 0,
	capacity = 60,
	hull_mass = 40,
	fuel_tank_mass = 20,
	thruster_fuel_use = 0.0004,
	price = 87000,
	hyperdrive_class = 3,
}