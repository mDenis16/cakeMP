#include <Common.h>

#include <Network/NetworkManager.h>

#include <Entities/LocalPlayer.h>
#include <Entities/Vehicle.h>

#include <System/Cake.h>
#include <Network/NetHandle.h>
#include <Network/NetworkEntityType.h>
#include <Network/Structs/CreatePed.h>
#include <Network/Structs/CreateVehicle.h>
#include <GTA/UI/UI.h>
#include <Utils/Formatting.h>

#include <enet/enet.h>

#include <shv/natives.h>


NAMESPACE_BEGIN;
//TODO: Perhaps this should move to some other place?
void NetworkManager::HandleMessage(NetworkMessage* message)
{
	if (message->m_type == NMT_Disconnect) {
		std::string reason;
		message->Read(reason);

		logWrite("Server disconnected us: '%s'", reason.c_str());

		uiNotify(reason);

		enet_peer_disconnect(m_localPeer, 0);
		m_localPeer = nullptr;

		return;
	}

	if (message->m_type == NMT_Handshake) {
		SAFE_MODIFY(m_playerInfos);

		NetHandle handle;
		glm::vec3 position;
		uint32_t skinHash;

		message->Read(handle);
		message->Read(position);
		message->Read(skinHash);

		std::string serverName;
		int serverMaxPlayers;

		message->Read(serverName);
		message->Read(serverMaxPlayers);

		m_currentServerName = serverName;
		m_currentServerMaxPlayers = serverMaxPlayers;

		logWrite("We have received our local handle: %u", handle.m_value);

		LocalPlayer& player = _pGame->m_player;
		player.GetPlayerInfo()->m_handle = handle;
		//	player.SetNetHandle(handle);
		player.SetModel(skinHash);
		//	player.SetPositionNoOffset(position);

		m_playerInfos.emplace_back(player.GetPlayerInfo());

		_pGame->OnConnected();

		return;
	}

	if (message->m_type == NMT_StreamIn) {
		SAFE_MODIFY(m_entitiesNetwork);


		logWrite("Streaming in:");

		uint32_t numEntities;
		message->Read(numEntities);

		for (uint32_t i = 0; i < numEntities; i++) {
			NetworkEntityType entityType = ET_None;
			NetHandle entityHandle;

			message->Read(entityType);
			message->Read(entityHandle);

			logWrite("  %u (%d)", entityHandle.m_value, (int)entityType);

			//TODO: Clean this up a bit, it's gonna become huge if we leave this like this
			if (entityType == ET_Player) {
				NetStructs::CreatePed createPedPlayer;

				message->Read(createPedPlayer);

				auto playerInfo = GetPlayer(entityHandle);

				Player* newPlayer = new Player(entityHandle, createPedPlayer);
				newPlayer->SetPlayerInfo(playerInfo);


				m_entitiesNetwork[entityHandle] = newPlayer;

			}
			else if (entityType == ET_Vehicle) {
				NetStructs::CreateVehicle createVehicle;

				message->Read(createVehicle);

				Vehicle* newVehicle = new Vehicle(entityHandle, createVehicle);
				m_entitiesNetwork[entityHandle] = newVehicle;

			}
			else {
				assert(false);
				break;
			}
		}
	}

	if (message->m_type == NMT_StreamOut) {
		logWrite("Streaming out:");

		SAFE_MODIFY(m_entitiesNetwork);


		uint32_t numEntities;
		message->Read(numEntities);

		for (uint32_t i = 0; i < numEntities; i++) {
			NetHandle handle;
			message->Read(handle);



			auto it = m_entitiesNetwork.find(handle);
			if (it == m_entitiesNetwork.end()) {
				assert(false);
				return;
			}

			logWrite("  %u (%d)", handle.m_value, (int)it->second->GetType());

			m_entitiesNetwork.erase(it);
			delete it->second;
		}
	}

	if (message->m_type == NMT_PlayerJoin) {
		SAFE_MODIFY(m_playerInfos);

		NetHandle handle;
		std::string username, nickname;

		message->Read(handle);
		message->Read(username);
		message->Read(nickname);

		assert(GetPlayer(handle) == nullptr);

		PlayerInfo* newPlayer = new PlayerInfo;
		newPlayer->m_handle = handle;
		newPlayer->m_username = username;
		newPlayer->m_nickname = nickname;
		m_playerInfos.emplace_back(newPlayer);

		logWrite("Player joined: %s (%s)", username.c_str(), nickname.c_str());

		uiNotify("~b~%s~s~ joined", nickname.c_str());

		return;
	}

	if (message->m_type == NMT_PlayerLeave) 
	{
		SAFE_MODIFY(m_playerInfos);

		NetHandle handle;

		message->Read(handle);

		auto player = GetPlayer(handle);
		assert(player != nullptr);

		logWrite("Player left: %s (%s)", player->m_username.c_str(), player->m_nickname.c_str());

		uiNotify("~b~%s~s~ left", player->m_nickname.c_str());

		std::remove(m_playerInfos.begin(), m_playerInfos.end(), player);

		return;
	}

	if (message->m_type == NMT_ChatMessage) {
		
		std::string text;

		message->Read(text);

		_pGame->m_interface.m_chat.AddMessage(text);

		return;
	}

	if (message->m_type == NMT_PlayerMove) {
		NetHandle handle;
	

		message->Read(handle);

		Player* player = GetEntityFromHandle<Player>(ET_Player, handle);
		if (player == nullptr) {
			// Can happen if the player has *just* been streamed out
			return;
		}

		if (player->m_inVehicle) {
			return;
		}

		auto record = new LagRecord();

		message->Read(record->position);
		message->Read(record->rotation);
		message->Read(record->velocity);
		message->Read(record->simulation_time);
		message->Read(record->tickcount);

		
		//player->m_speedOnFoot = (OnFootMoveTypes)newMoveType;

		SAFE_MODIFY(player->lagRecords);

		player->lagRecords.push_back(record);


		return;
	}

	if (message->m_type == NMT_Weather) {
		std::string weatherType;

		message->Read(weatherType);

		GAMEPLAY::SET_WEATHER_TYPE_PERSIST(weatherType.c_str());

		return;
	}

	if (message->m_type == NMT_ClockTime) {
		int hours, minutes, seconds;

		message->Read(hours);
		message->Read(minutes);
		message->Read(seconds);

		TIME::SET_CLOCK_TIME(hours, minutes, seconds);

		return;
	}

	if (message->m_type == NMT_EnteringVehicle) {
		NetHandle playerHandle, vehicleHandle;
		int seat;

		message->Read(playerHandle);
		message->Read(vehicleHandle);
		message->Read(seat);

		Player* player = GetEntityFromHandle<Player>(ET_Player, playerHandle);
		Vehicle* vehicle = GetEntityFromHandle<Vehicle>(ET_Vehicle, vehicleHandle);

		if (player == nullptr || vehicle == nullptr) {
			logWrite("WARNING: Player %p (%u) tried entering vehicle %p (%u) (entering)", player, (uint32_t)playerHandle, vehicle, (uint32_t)vehicleHandle);
			return;
		}

		player->m_inVehicle = true;
		AI::TASK_ENTER_VEHICLE(player->GetLocalHandle(), vehicle->GetLocalHandle(), 1000, seat, 1.0f, 3, 0);

		return;
	}

	if (message->m_type == NMT_EnteredVehicle) {
		NetHandle playerHandle, vehicleHandle;
		int seat;

		message->Read(playerHandle);
		message->Read(vehicleHandle);
		message->Read(seat);

		Player* player = GetEntityFromHandle<Player>(ET_Player, playerHandle);
		Vehicle* vehicle = GetEntityFromHandle<Vehicle>(ET_Vehicle, vehicleHandle);

		if (player == nullptr || vehicle == nullptr) {
			logWrite("WARNING: Player %p (%u) tried entering vehicle %p (%u) (entered)", player, (uint32_t)playerHandle, vehicle, (uint32_t)vehicleHandle);
			return;
		}

		PED::SET_PED_INTO_VEHICLE(player->GetLocalHandle(), vehicle->GetLocalHandle(), seat);

		return;
	}

	if (message->m_type == NMT_LeftVehicle) {
		NetHandle playerHandle, vehicleHandle;
		int seat;

		message->Read(playerHandle);
		message->Read(vehicleHandle);
		message->Read(seat);

		Player* player = GetEntityFromHandle<Player>(ET_Player, playerHandle);
		Vehicle* vehicle = GetEntityFromHandle<Vehicle>(ET_Vehicle, vehicleHandle);

		if (player == nullptr || vehicle == nullptr) {
			logWrite("WARNING: Player %p (%u) tried leaving vehicle %p (%u)", player, (uint32_t)playerHandle, vehicle, (uint32_t)vehicleHandle);
			return;
		}

		player->m_inVehicle = false;

		//TODO: Somehow wait for this task to finish before allowing movement (like m_inVehicle), to avoid janky animation
		AI::TASK_LEAVE_VEHICLE(player->GetLocalHandle(), vehicle->GetLocalHandle(), 1);

		return;
	}
}



void NetworkManager::MessageHandler()
{
	SAFE_MODIFY(m_incomingMessages);

	while (m_incomingMessages.size() > 0) {
		NetworkMessage* message = m_incomingMessages.front();
		m_incomingMessages.pop();

		logWrite("Incoming message of size %u at %p (type %d)", message->m_length, message->m_data, (int)message->m_type);

		message->m_handled = true;

		HandleMessage(message);

		if (message->m_handled) {
			delete message;
		}
	}
}



NAMESPACE_END;