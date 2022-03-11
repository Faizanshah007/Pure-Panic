#include "NetworkedGame.h"
#include "NetworkedPlayer.h"
#include "../CSC8503Common/GameServer.h"
#include "../CSC8503Common/GameClient.h"
#include "../GameTech/LevelLoader.h"
#include "../CSC8503Common/InputHandler.h"

#define COLLISION_MSG 30

using namespace NCL;
using namespace CSC8503;

NetworkedGame* NetworkedGame::instance = nullptr;

#ifndef _ORBIS
struct MessagePacket : public GamePacket {
	short playerID;
	short messageID;

	MessagePacket() {
		type = Message;
		size = sizeof(short) * 2;
	}
};
#endif

NetworkedGame::NetworkedGame() {
	thisServer = nullptr;
	thisClient = nullptr;

#ifndef _ORBIS
	NetworkBase::Initialise();
#endif
	timeToNextPacket = 0.0f;
	packetsToSnapshot = 0;

	instance = this;
}

NetworkedGame::~NetworkedGame() {
	delete thisServer;
	delete thisClient;
}

void NetworkedGame::StartAsServer() {
#ifndef _ORBIS
	thisServer = new GameServer(NetworkBase::GetDefaultPort(), 4);

	thisServer->RegisterPacketHandler(Received_State, this);

	SpawnPlayer();
	StartLevel();
#endif
}

void NetworkedGame::StartAsClient(char a, char b, char c, char d) {
#ifndef _ORBIS
	thisClient = new GameClient();
	thisClient->Connect(a, b, c, d, NetworkBase::GetDefaultPort());

	thisClient->RegisterPacketHandler(Delta_State, this);
	thisClient->RegisterPacketHandler(Full_State, this);
	thisClient->RegisterPacketHandler(Fire_State, this);
	thisClient->RegisterPacketHandler(Player_Connected, this);
	thisClient->RegisterPacketHandler(Player_Disconnected, this);
	thisClient->RegisterPacketHandler(Assign_ID, this);
#endif
}

void NetworkedGame::UpdateGame(float dt) {
	timeToNextPacket -= dt;
	if (timeToNextPacket < 0) {
		if (thisServer) {
			UpdateAsServer(dt);
		}
		else if (thisClient) {
			UpdateAsClient(dt);
		}
		timeToNextPacket += 1.0f / 60.0f; //60hz server/client update
	}

#ifndef _ORBIS
	if (!thisServer && Window::GetKeyboard()->KeyPressed(KeyboardKeys::F9)) {
		StartAsServer();
		std::cout << "Server start" << std::endl;
	}
	if (!thisClient && Window::GetKeyboard()->KeyPressed(KeyboardKeys::F10)) {
		StartAsClient(127, 0, 0, 1);
		std::cout << "Client start" << std::endl;
	}
#endif

	TutorialGame::UpdateGame(dt);
}

void NetworkedGame::UpdateAsServer(float dt) {
#ifndef _ORBIS
	thisServer->UpdateServer();

	if (localPlayer->IsFiring()) {
		FirePacket* newPacket = new FirePacket();
		newPacket->clientID = localPlayer->GetNetworkObject()->GetNetID();
		newPacket->pitch = localPlayer->GetCam()->GetPitch();
		thisServer->SendGlobalPacket(*newPacket);
		delete newPacket;
	}
#endif

	BroadcastSnapshot();
}

void NetworkedGame::UpdateAsClient(float dt) {
#ifndef _ORBIS
	thisClient->UpdateClient();

	if (!localPlayer)
		return;

	ClientPacket newPacket;
	newPacket.clientID = playerID;
	clientLastPacketID++;
	newPacket.lastID = clientLastPacketID;

	Vector3 position = localPlayer->GetTransform().GetPosition();
	newPacket.pos[0] = position.x;
	newPacket.pos[1] = position.y;
	newPacket.pos[2] = position.z;

	newPacket.yaw = localPlayer->GetTransform().GetOrientation().ToEuler().y;
	newPacket.pitch = localPlayer->GetCam()->GetPitch();

	newPacket.firing = localPlayer->IsFiring();

	thisClient->SendPacket(newPacket);
#endif
}

void NetworkedGame::BroadcastSnapshot() {
	std::vector<GameObject*>::const_iterator first;
	std::vector<GameObject*>::const_iterator last;

	world->GetObjectIterators(first, last);

	for (auto i = first; i != last; i++) {
		NetworkObject* o = (*i)->GetNetworkObject();

		if (!o || o == nullptr) {
			continue;
		}
#ifndef _ORBIS
		GamePacket* newPacket = nullptr;
		if (o->WritePacket(&newPacket)) {
			thisServer->SendGlobalPacket(*newPacket);
			delete newPacket;
		}
#endif
	}
}

void NetworkedGame::SpawnPlayer() {
	localPlayer = player1;
#ifndef _ORBIS
	localPlayer->SetNetworkObject(new NetworkObject(*localPlayer, playerID, world));
#endif
	localPlayer->GetPhysicsObject()->SetDynamic(true);
	localPlayer->GetTransform().SetPosition(Vector3(playerID * 5, 10, 0));
}

void NetworkedGame::StartLevel() {
	// Reset the level
	// Start timer
}
#ifndef _ORBIS
void NetworkedGame::ReceivePacket(int type, GamePacket* payload, int source) {
	//SERVER version of the game will receive these from the clients
	if (type == Received_State) {
		ClientPacket* realPacket = (ClientPacket*)payload;
		HandleClientPacket(realPacket);
		return;
	}

	//CLIENT version of the game will receive these from the servers
	// Connection / Initialisation packets
	switch (type) {
	case(Assign_ID):
		HandleAssignID((AssignIDPacket*)payload);
		break;
	case(Player_Connected):
		HandlePlayerConnect((NewPlayerPacket*)payload);
		break;
	case(Player_Disconnected):
		HandlePlayerDisconnect((PlayerDisconnectPacket*)payload);
		break;
	}

	if (!CheckExists((IDPacket*)payload))
		return;

	switch (type) {
	case(Full_State):
		HandleFullState((FullPacket*)payload);
		break;
	case(Fire_State):
		HandleFireState((FirePacket*)payload);
		break;
	}
}

void NetworkedGame::OnPlayerCollision(NetworkPlayer* a, NetworkPlayer* b) {
	if (thisServer) { //detected a collision between players!
		MessagePacket newPacket;
		newPacket.messageID = COLLISION_MSG;
		newPacket.playerID = a->GetPlayerNum();

		thisClient->SendPacket(newPacket);

		newPacket.playerID = b->GetPlayerNum();
		thisClient->SendPacket(newPacket);
	}
}

void NetworkedGame::HandleClientPacket(ClientPacket* packet)
{
	auto index = clientHistory.find(packet->clientID);

	if (index != clientHistory.end()) {
		if (index->second < packet->lastID) {
			index->second = packet->lastID;
			auto obj = serverPlayers.find(packet->clientID);
			Vector3 pos(packet->pos[0], packet->pos[1], packet->pos[2]);
			obj->second->GetTransform().SetPosition(pos);
			obj->second->GetTransform().SetOrientation(Quaternion::EulerAnglesToQuaternion(0, packet->yaw, 0));
			if (packet->firing) {
				Fire(obj->second, packet->pitch, packet->clientID);
			}
		}
	}
	else {
		AddNewPlayerToServer(packet->clientID, packet->lastID);
	}
}

void NetworkedGame::AddNewPlayerToServer(int clientID, int lastID)
{
	clientHistory.insert(std::pair<int, int>(clientID, lastID));

	GameObject* client = levelLoader->AddDummyPlayerToWorld(Vector3(clientID * 5, 10, 0));
	client->SetNetworkObject(new NetworkObject(*client, clientID, world));
	client->GetPhysicsObject()->SetDynamic(true);
	client->GetPhysicsObject()->SetGravity(false);

	serverPlayers.insert(std::pair<int, Player*>(clientID, (Player*)client));

	if (!(clientID < networkObjects.size()))
		networkObjects.resize(clientID + 1);
	networkObjects[clientID] = client->GetNetworkObject();
	std::cout << "New player added to server: ClientID (" << clientID << "), LastID(" << lastID << ")\n";
}

void NetworkedGame::Fire(GameObject* owner, float pitch, int clientID)
{
	levelLoader->SpawnProjectile(owner, pitch);
	FirePacket newPacket;
	newPacket.clientID = clientID;
	newPacket.pitch = pitch;
	thisServer->SendGlobalPacket(newPacket); 
}

void NetworkedGame::HandleFullState(FullPacket* packet)
{
	if (packet->clientID == playerID)
		return;

	if (packet->clientID < (int)networkObjects.size()) {
		if (networkObjects[packet->clientID])
			networkObjects[packet->clientID]->ReadPacket(*packet);
	}
}

bool NCL::CSC8503::NetworkedGame::CheckExists(IDPacket* packet)
{
	if (packet->clientID == playerID)
		return false;
	if (!(packet->clientID < networkObjects.size())) {
		networkObjects.resize(packet->clientID + 1);
	}
	if (!networkObjects[packet->clientID]) {
		GameObject* p = levelLoader->AddDummyPlayerToWorld(Vector3(packet->clientID * 5, 10, 0));
		p->GetPhysicsObject()->SetDynamic(true);
		p->GetPhysicsObject()->SetGravity(false);
		p->SetNetworkObject(new NetworkObject(*p, packet->clientID, world));
		networkObjects[packet->clientID] = p->GetNetworkObject();
	}
	return true;
}

void NetworkedGame::HandleFireState(FirePacket* packet)
{
	if (packet->clientID == playerID)
		return;
	auto obj = networkObjects[packet->clientID];
	if (obj)
		levelLoader->SpawnProjectile(&obj->object, packet->pitch);
}

void NetworkedGame::HandleAssignID(AssignIDPacket* packet)
{
	std::cout << "ID Assigned: " << packet->clientID << std::endl;
	playerID = packet->clientID;
	SpawnPlayer();
}

void NetworkedGame::HandlePlayerConnect(NewPlayerPacket* packet)
{
	std::cout << "Client: New player connected!" << std::endl;
	std::cout << "_Player ID: " << packet->clientID << std::endl;

	if (packet->clientID != playerID) {
		GameObject* newPlayer = levelLoader->AddDummyPlayerToWorld(Vector3(10, 15, 10));
		newPlayer->SetNetworkObject(new NetworkObject(*newPlayer, packet->clientID, world));
		newPlayer->GetPhysicsObject()->SetDynamic(true);
		std::cout << "Player Spawned with Network ID: " << newPlayer->GetNetworkObject()->GetNetID() << "." << std::endl;
		if (!(packet->clientID < networkObjects.size())) {
			networkObjects.resize(packet->clientID + 1);
		}
		networkObjects[newPlayer->GetNetworkObject()->GetNetID()] = (newPlayer->GetNetworkObject());
	}
}

void NetworkedGame::HandlePlayerDisconnect(PlayerDisconnectPacket* packet) {
	RemovePlayerFromServer(packet->clientID);
}

void NetworkedGame::RemovePlayerFromServer(int clientID) {
	delete networkObjects[clientID];
	networkObjects[clientID] = nullptr;
}
#endif