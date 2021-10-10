#pragma once

#include <Common.h>

#include <Network/NetHandle.h>

#include <Utils/Interpolator.h>
#include <Network/NetworkEntityType.h>
#include <Network/NetworkMessage.h>

NAMESPACE_BEGIN;
class LagRecord
{
public:
	glm::vec3 position;
	glm::vec3 rotation;
	glm::vec3 velocity;
	float simulation_time = 0.f;
	int tickcount = 0;
};

class Entity
{
private:
	int m_handle = 0;
	NetHandle m_netHandle;

protected:


public:


	SAFE_PROP(std::vector<LagRecord*>, lagRecords)


	


	Entity();
	Entity(int localHandle, const NetHandle &netHandle);
	virtual ~Entity();

	virtual NetworkEntityType GetType() = 0;




	virtual bool IsLocal();
	virtual bool CanBeDeleted();

	virtual int GetLocalHandle();
	virtual void SetLocalHandle(int handle);

	virtual NetHandle GetNetHandle();
	virtual void SetNetHandle(const NetHandle &handle);

	virtual void Delete();

	virtual bool IsDead();

	virtual glm::vec3 GetPosition();
	virtual void SetPosition(const glm::vec3 &pos);
	virtual void SetPositionNoOffset(const glm::vec3 &pos);

	virtual glm::vec3 GetRotation();
	virtual void SetRotation(const glm::vec3 &rot);

	virtual glm::quat GetQuat();
	virtual void SetQuat(const glm::quat &quat);

	virtual float GetHeading();
	virtual void SetHeading(float heading);

	virtual glm::vec3 GetVelocity();
	virtual void SetVelocity(const glm::vec3 &vel);

	virtual uint32_t GetModel();


	virtual void OnNetworkUpdate(NetworkMessage* message) = 0;
};

NAMESPACE_END;
