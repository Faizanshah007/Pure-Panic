#pragma once
#ifdef _ORBIS
#define ORBISNET
#endif

#ifndef ORBISNET
#include <enet/enet.h>
#endif
#ifndef _ORBIS
#include <winsock2.h>
#endif

#include <map>
#include <string>

enum BasicNetworkMessages {
	No_Message,
	Hello,
	Message,
	String_Message,
	Delta_State,	//1 byte per channel since the last state
	Full_State,		//Full transform etc
	Received_State, //received from a client, informs that its received packet n
	Fire_State,
	PowerUp_State,
	Player_Connected,
	Player_Disconnected,
	Assign_ID,
	Ack,
	Shutdown
};

struct GamePacket {
	short size;
	short type;

	GamePacket() {
		type = BasicNetworkMessages::No_Message;
		size = 0;
	}

	GamePacket(short type) : GamePacket() {
		this->type = type;
	}

	int GetTotalSize() {
		return sizeof(GamePacket) + size;
	}
};

struct IDPacket : public GamePacket {
	int clientID;
};

struct StringPacket : public GamePacket {
	char	stringData[256];

	StringPacket(const std::string& message) {
		type = BasicNetworkMessages::String_Message;
		size = (short)message.length();

		memcpy(stringData, message.data(), size);
	};

	std::string GetStringFromData() {
		std::string realString(stringData);
		realString.resize(size);
		return realString;
	}
};

struct NewPlayerPacket : public IDPacket {
	NewPlayerPacket(int p) {
		type = BasicNetworkMessages::Player_Connected;
		clientID = p;
		size = sizeof(int);
	}
};

struct AssignIDPacket : public IDPacket {
	int networkID;
	AssignIDPacket(int p) {
		type = BasicNetworkMessages::Assign_ID;
		clientID = p;
		size = sizeof(int) * 2;
	}
};

struct PlayerDisconnectPacket : public IDPacket {
	PlayerDisconnectPacket(int p) {
		type = BasicNetworkMessages::Player_Disconnected;
		clientID = p;
		size = sizeof(int);
	}
};

class PacketReceiver {
public:
	virtual void ReceivePacket(int type, GamePacket* payload, int source = -1) = 0;
};

class NetworkBase {
public:
	static void Initialise();
	static void Destroy();

	static int GetDefaultPort() {
		return 1234;
	}

	void RegisterPacketHandler(int msgID, PacketReceiver* receiver) {
		packetHandlers.insert(std::make_pair(msgID, receiver));
	}
protected:
	NetworkBase();
	~NetworkBase();

	bool ProcessPacket(GamePacket* p, int peerID = -1);

	typedef std::multimap<int, PacketReceiver*>::const_iterator PacketHandlerIterator;

	bool GetPacketHandlers(int msgID, PacketHandlerIterator& first, PacketHandlerIterator& last) const {
		auto range = packetHandlers.equal_range(msgID);

		if (range.first == packetHandlers.end()) {
			return false; //no handlers for this message type!
		}
		first = range.first;
		last = range.second;

		return true;
	}

#ifndef ORBISNET
	ENetHost* netHandle;
#endif

	std::multimap<int, PacketReceiver*> packetHandlers;
};