#pragma once

#include <Common.h>

NAMESPACE_BEGIN;

class Settings
{
public:
	std::string ConnectToHost = "192.168.88.252";
	uint16_t ConnectToPort = 22005;

	std::string Nickname;

public:
	Settings();
};

NAMESPACE_END;
