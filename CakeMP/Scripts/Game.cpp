#include <Common.h>
#include <Scripts/Game.h>

#include <System/Cake.h>

#include <shv/main.h>
#include <shv/natives.h>

NAMESPACE_BEGIN;
void scriptGameFrame()
{
	while (DLC2::GET_IS_LOADING_SCREEN_ACTIVE()) {
		WAIT(0);
	}

	if (_pGame && _pGame->m_initialized)
	{
		while(_pGame->m_initialized)
			_pGame->Frame();

		WAIT(0);
	}
}

void scriptGame()
{
	while (DLC2::GET_IS_LOADING_SCREEN_ACTIVE()) {
		WAIT(0);
	}

	_pGame->Initialize();

	/*
	UI::_SET_LOADING_PROMPT_TEXT_ENTRY("STRING");
	UI::ADD_TEXT_COMPONENT_SUBSTRING_PLAYER_NAME("Loading " PROJECT_NAME);
	UI::_SHOW_LOADING_PROMPT(3);
	WAIT(1000);
	UI::_REMOVE_LOADING_PROMPT();
	*/

	const int UpdateInterval = (int)((1000 / 40));

	ClockTime tmLastUpdate = Clock::now();

	while (true){
		_pGame->Update(0);
	}
}

NAMESPACE_END;
