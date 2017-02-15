#include <Common.h>

#include <Entities/LocalPlayer.h>

#include <System/Strawberry.h>
#include <Network/NetworkMessage.h>

#include <shv/natives.h>

NAMESPACE_BEGIN;

LocalPlayer::LocalPlayer()
{
}

LocalPlayer::~LocalPlayer()
{
}

bool LocalPlayer::CanBeDeleted()
{
	return false;
}

int LocalPlayer::GetLocalHandle()
{
	return PLAYER::GET_PLAYER_PED(Ped::GetLocalHandle());
}

void LocalPlayer::SetLocalHandle(int handle)
{
	m_playerHandle = handle;
}

void LocalPlayer::Delete()
{
}

void LocalPlayer::SetModel(uint32_t hash)
{
	if (!mdlRequest(hash)) {
		assert(false);
		return;
	}
	PLAYER::SET_PLAYER_MODEL(m_playerHandle, hash);
}

void LocalPlayer::Initialize()
{
	SetLocalHandle(PLAYER::GET_PLAYER_INDEX());

	m_username = PLAYER::GET_PLAYER_NAME(m_playerHandle);
	m_nickname = m_username; //TODO
}

void LocalPlayer::Update()
{
	if (_pGame->m_network.m_connected) {
		glm::vec3 pos = GetPosition();

		if (glm::distance(m_lastSyncedPosition, pos) > 0.5f) {
			glm::vec3 rot = GetRotation();
			glm::vec3 vel = GetVelocity();

			m_lastSyncedPosition = pos;

			uint8_t moveType = 0;
			if (AI::IS_PED_WALKING(GetLocalHandle())) {
				moveType = 1;
			} else if (AI::IS_PED_RUNNING(GetLocalHandle())) {
				moveType = 2;
			} else if (AI::IS_PED_SPRINTING(GetLocalHandle())) {
				moveType = 3;
			}

			NetworkMessage* msgPos = new NetworkMessage(NMT_PlayerMove);
			msgPos->Write(pos);
			msgPos->Write(rot.z);
			msgPos->Write(vel);
			msgPos->Write(moveType);
			_pGame->m_network.SendToHost(msgPos);
		}
	}
}

NAMESPACE_END;
