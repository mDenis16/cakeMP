#pragma once

#include <Common.h>

class Settings
{
public:
	std::string ListenHost = "192.168.88.252";
	uint16_t ListenPort = 22005;

	std::string ServerName = "Cake is good";

	uint32_t MaxClients = 100;

	uint32_t TickRate = 60;

	float StreamingRange = 250.0f;

public:
	Settings();
};
