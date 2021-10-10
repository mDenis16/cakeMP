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

static void networkMessageFree(ENetPacket* packet)
{
	assert(packet->userData != nullptr);
	assert(((NetworkMessage*)packet->userData)->m_outgoing);
	delete (NetworkMessage*)packet->userData;
}

NetworkManager::NetworkManager()
{
}

NetworkManager::~NetworkManager()
{
	if (m_localPeer != nullptr) {
		Disconnect();
	}

	if (m_localHost != nullptr) {
		enet_host_destroy(m_localHost);
		enet_deinitialize();
	}
}

void NetworkManager::Connect(const char* hostname, uint16_t port)
{
	if (m_localHost == nullptr || m_localPeer != nullptr) {
		return;
	}

	logWrite("Connecting to %s:%u", hostname, port);

	uiNotify("Connecting..");

	ENetAddress addr;
	enet_address_set_host(&addr, hostname);
	addr.port = port;
	m_localPeer = enet_host_connect(m_localHost, &addr, 1, 0);
}

void NetworkManager::Disconnect()
{
	if (m_localPeer == nullptr) {
		return;
	}

	logWrite("Disconnecting from %08x", m_localPeer->address.host);

	uiNotify("Disconnecting..");

	enet_peer_disconnect(m_localPeer, 0);
	m_localPeer = nullptr;
}

bool NetworkManager::IsConnecting()
{
	return m_localPeer != nullptr && !m_connected;
}

bool NetworkManager::IsConnected()
{
	return m_connected;
}

void NetworkManager::ClearEntities()
{
	SAFE_MODIFY(m_entitiesNetwork);

	for (auto &pair : m_entitiesNetwork) 
		delete pair.second;
	
	m_entitiesNetwork.clear();
}

int NetworkManager::GetEntityCount()
{
	return (int)m_entitiesNetwork.size();
}

void NetworkManager::SendToHost(NetworkMessage* message)
{
	if (m_localPeer == nullptr) {
		logWrite("WARNING: Tried sending a network message with type %d while not connected!", (int)message->m_type);
		delete message;
		return;
	}

	SAFE_MODIFY(m_outgoingMessages);

	m_outgoingMessages.push(message);
}

Entity* NetworkManager::GetEntityFromHandle(NetworkEntityType expectedType, const NetHandle &handle)
{
	SAFE_READ(m_entitiesNetwork);

	auto it = m_entitiesNetwork.find(handle);
	if (it == m_entitiesNetwork.end()) {
		assert(false);
		return nullptr;
	}

	if (it->second->GetType() != expectedType) {
		return nullptr;
	}

	return it->second;
}

Entity* NetworkManager::GetEntityFromLocalHandle(NetworkEntityType expectedType, int handle)
{
	SAFE_READ(m_entitiesNetwork);

	for (auto &pair : m_entitiesNetwork) {
		if (pair.second->GetLocalHandle() == handle && pair.second->GetType() == expectedType) {
			return pair.second;
		}
	}
	return nullptr;
}

void NetworkManager::Initialize()
{
	if (enet_initialize() < 0) {
		logWrite("Failed to initialize ENet!");
		return;
	}

	m_localHost = enet_host_create(nullptr, 1, 1, 0, 0);

	this->NetworkThread = new std::thread(std::bind(&NetworkManager::Update, this));
	this->MessageHandlerThread = new std::thread(std::bind(&NetworkManager::MessageHandler, this));

}

void NetworkManager::Update()
{
	if (m_localHost == nullptr) {
		return;
	}


	uint32_t incomingBytes = 0;
	
	ENetEvent ev;
	while (enet_host_service(m_localHost, &ev, 0) > 0) {
		if (ev.type == ENET_EVENT_TYPE_CONNECT) {
			//m_localPeer = ev.peer;
			m_connected = true;

			LocalPlayer &player = _pGame->m_player;

			auto playerInfo = player.GetPlayerInfo();

			NetworkMessage* msgHandshake = new NetworkMessage(NMT_Handshake);
			msgHandshake->Write(playerInfo->m_username);
			msgHandshake->Write(playerInfo->m_nickname);
			SendToHost(msgHandshake);

			//NOTE: We don't call OnConnected until the server has accepted our handshake

		} else if (ev.type == ENET_EVENT_TYPE_DISCONNECT) {
			m_localPeer = nullptr;
			m_connected = false;

			_pGame->OnDisconnected();

		} else if (ev.type == ENET_EVENT_TYPE_RECEIVE) {
			SAFE_MODIFY(m_incomingMessages);

			NetworkMessage* newMessage = new NetworkMessage(ev.peer, ev.packet);
			incomingBytes += (uint32_t)newMessage->m_length;

			
			m_incomingMessages.push(newMessage);
		}
	}

	m_statsIncomingMessagesTotal += m_incomingMessages.size();
	m_statsIncomingMessages.Add((uint32_t)m_incomingMessages.size());
	m_statsIncomingBytes.Add(incomingBytes);
	m_statsIncomingBytesTotal += incomingBytes;



	m_statsOutgoingMessagesTotal += m_outgoingMessages.size();
	m_statsOutgoingMessages.Add((uint32_t)m_outgoingMessages.size());

	uint32_t outgoingBytes = 0;

	while (m_outgoingMessages.size() > 0) {
		NetworkMessage* message = m_outgoingMessages.front();
		m_outgoingMessages.pop();

		outgoingBytes += (uint32_t)message->m_length;

		ENetPacket* newPacket = enet_packet_create(message->m_data, message->m_length, ENET_PACKET_FLAG_RELIABLE | ENET_PACKET_FLAG_NO_ALLOCATE);
		newPacket->userData = message;
		newPacket->freeCallback = networkMessageFree;
		enet_peer_send(m_localPeer, 0, newPacket);

		// Note: We don't delete this packet here, we wait until ENet tells us it's no longer in use and delete it in the free callback (networkMessageFree)
	}

	m_statsOutgoingBytes.Add(outgoingBytes);
	m_statsOutgoingBytesTotal += outgoingBytes;
}

s2::ref<PlayerInfo> NetworkManager::GetPlayer(const NetHandle &handle)
{
	auto it = std::find_if(m_playerInfos.begin(), m_playerInfos.end(), [&handle](s2::ref<PlayerInfo> &player) {
		return player->m_handle == handle;
	});

	if (it != m_playerInfos.end()) {
		return *it;
	}

	return nullptr;
}

s2::ref<PlayerInfo> NetworkManager::GetPlayerByIndex(int index)
{
	return m_playerInfos[index];
}

int NetworkManager::GetPlayerCount()
{
	return (int)m_playerInfos.size();
}

int NetworkManager::GetServerMaxPlayers()
{
	return m_currentServerMaxPlayers;
}

std::string NetworkManager::GetServerIP()
{
	if (!IsConnected()) {
		return "";
	}

	uint32_t addr = m_localHost->address.host;
	uint16_t port = m_localHost->address.port;

	return fmtString("%u.%u.%u.%u:%u",
		((addr & 0x000000FF) >> 0),
		((addr & 0x0000FF00) >> 8),
		((addr & 0x00FF0000) >> 16),
		((addr & 0xFF000000) >> 24),
		port
	);
}

std::string NetworkManager::GetServerName()
{
	return m_currentServerName;
}


NAMESPACE_END;
