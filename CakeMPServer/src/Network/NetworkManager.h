#pragma once

#include <Common.h>

#include <Network/NetworkMessage.h>
#include <Network/NetHandle.h>
#include <Entities/Player.h>

#include <enet/enet.h>

class GameServer;

class NetworkManager
{
public:
	GameServer* m_server = nullptr;

	ENetHost* m_hostListen = nullptr;

	void QueueExecutor();

	void MessageQueue();

	void QueueSender();

	void Update();




	SAFE_PROP(std::queue<NetworkMessage*>, m_incomingMessages);
	SAFE_PROP(std::queue<NetworkMessage*>, m_outgoingMessages);

	SAFE_PROP(std::vector<Player*>, m_players);


	std::mutex QueueExecutorMutex;
	std::condition_variable QueueExecutorMutexCond;
	bool QueueExecutorMutexCondFlag = false;

	std::thread* QueueExecutorThread = nullptr;



	std::thread* MessageQueueThread = nullptr;


	std::thread* QueueSenderThread = nullptr;
	std::mutex QueueSenderMutex;
	std::condition_variable QueueSenderMutexCond;
	bool QueueSenderMutexCondFlag = false;


	uint32_t m_handleIterator = 1;

public:
	NetworkManager(GameServer* server);
	~NetworkManager();

	

	NetHandle AssignHandle();

	void Listen(const char* host, uint16_t port, uint32_t maxClients);
	void Close();

	void OnClientConnect(ENetPeer* peer);
	void OnClientDisconnect(ENetPeer* peer);

	//TODO: Deprecate these functions and have only one SendMessage() instead
	void SendMessageTo(ENetPeer* peer, NetworkMessage* message);
	void SendMessageToAll(NetworkMessage* message);
	void SendMessageToAll(NetworkMessage* message, ENetPeer* except);
	void SendMessageToRange(const glm::vec3 &pos, float range, NetworkMessage* message);
	void SendMessageToRange(const glm::vec3 &pos, float range, NetworkMessage* message, ENetPeer* except);


};
