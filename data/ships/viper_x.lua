-- Copyright © 2008-2012 Pioneer Developers. See AUTHORS.txt for details
-- Licensed under the terms of CC-BY-SA 3.0. See licenses/CC-BY-SA-3.0.txt

define_ship {
	name='Viper X',
	model='viper_x',
	forward_thrust = 9e6,
	reverse_thrust = 3e6,
	up_thrust = 3e6,
	down_thrust = 2e6,
	left_thrust = 2e6,
	right_thrust = 2e6,
	angular_thrust = 20e6,
	camera_offset = v(4,2,-1),
	gun_mounts =
	{
		{ v(0,-1.4,-28), v(0,0,-1), 5, 'HORIZONTAL' },
		{ v(0,0,0), v(0,0,1), 5, 'HORIZONTAL' }
	},
	max_cargo = 55,
	max_laser = 1,
	max_missile = 4,
	max_cargoscoop = 0,
	max_fuelscoop = 1,
	capacity = 55,
	hull_mass = 25,
	fuel_tank_mass = 15,
	thruster_fuel_use = 0.0002,
	price = 90000,
	hyperdrive_class = 2,
}
