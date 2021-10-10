#pragma once

class LagRecord
{
public:
	glm::vec3 m_position;
	glm::vec3 m_velocity;
	glm::vec3 m_rotation;
	uint8_t m_movetype;
	float heading = 0.f;
	float simulation_time = 0.f;
	int tickcount = 0;
};

