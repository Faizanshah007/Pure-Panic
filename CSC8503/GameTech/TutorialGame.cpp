#include "TutorialGame.h"
#include "../CSC8503Common/GameWorld.h"
#include "../../Plugins/OpenGLRendering/OGLMesh.h"
#include "../../Plugins/OpenGLRendering/OGLShader.h"
#include "../../Plugins/OpenGLRendering/OGLTexture.h"
#include "../../Common/TextureLoader.h"
#include "../../Common/Quaternion.h"

#include "..//CSC8503Common/InputHandler.h"
#include "..//CSC8503Common/GameActor.h"
#include "..//CSC8503Common/Command.h"

using namespace NCL;
using namespace CSC8503;

TutorialGame::TutorialGame()	{
	world		= new GameWorld();
	renderer	= new GameTechRenderer(*world);
	physics		= new PhysicsSystem(*world);

	forceMagnitude	= 30.0f;
	useGravity		= true;
	physics->UseGravity(useGravity);

	inSelectionMode = false;
	physics->UseGravity(true);

	testStateObject = nullptr;

	state = PLAY;

	Debug::SetRenderer(renderer);

	//physics->SetGravity(Vector3(0, 9.8f, 0));
	//physics->SetLinearDamping(10.0f);

#pragma region Commands

	/*
		Command Design Pattern Explanation

		The command design pattern I've implemented comes down to three classes, all of which can be found in the Input Handling filter:
		Command.h:
			This contains all the commands you will use to affect the world (via input handling). There are a few examples
			in the class itself of how to use them.

		GameActor.h:
			This is a gameobject child that you will control using inputs, I've given the class a set of default methods, which
			can be overriden if you derive a Player class from GameActor for example. Use the commands class above to attach methods
			to commands, which can in turn be assigned to keys in the input handler.

		InputHandler.h:
			This is where keys are assigned, each key you want to assign is made as a Command* pointer variable. The handleInputs method
			is called every frame, it will loop through each of the keys, see if they are assigned to a command, if so it checks if the key is
			pressed and executes accordingly. 

		Basically, all this means is that the hard coding of key checking is done in a separate file, and commands are kept in their own file, 
		so everything is organised and neat. 

		To use this, as shown in the example below, you need to instantiate an input handler and some commands, and bind the commands to the buttons.
		It makes it very easy and readable to change which keys do what. If there is a specific gameobject you wish to register inputs for, you need
		to instantiate a GameActor and pass it into the Commands accordingly.
			
	*/

	// Character movement Go to Line 354

	inputHandler = new InputHandler();

	Command* toggleGrav = new ToggleGravityCommand(physics);
	Command* toggleDebug = new ToggleBoolCommand(&debugDraw);
	
	inputHandler->BindButtonG(toggleGrav);
	inputHandler->BindButtonJ(toggleDebug);

#pragma endregion

	InitialiseAssets();
}

/*

Each of the little demo scenarios used in the game uses the same 2 meshes, 
and the same texture and shader. There's no need to ever load in anything else
for this module, even in the coursework, but you can add it if you like!

*/
void TutorialGame::InitialiseAssets() {
	auto loadFunc = [](const string& name, OGLMesh** into) {
		*into = new OGLMesh(name);
		(*into)->SetPrimitiveType(GeometryPrimitive::Triangles);
		(*into)->UploadToGPU();
	};

	loadFunc("cube.msh"		 , &cubeMesh);
	loadFunc("sphere.msh"	 , &sphereMesh);
	loadFunc("Male1.msh"	 , &charMeshA);
	loadFunc("courier.msh"	 , &charMeshB);
	loadFunc("security.msh"	 , &enemyMesh);
	loadFunc("coin.msh"		 , &bonusMesh);
	loadFunc("capsule.msh"	 , &capsuleMesh); 
	loadFunc("Corridor_Floor_Basic.msh"	 , &corridorFloor);
	corridorFloorTex = (OGLTexture*)TextureLoader::LoadAPITexture("Corridor_Light_Colour.png");
	loadFunc("Corridor_Wall_Alert.msh"	 , &corridorWallAlert);
	corridorWallAlertTex = (OGLTexture*)TextureLoader::LoadAPITexture("corridor_wall_c.png");
	loadFunc("Corridor_Wall_Corner_In_Both.msh"	 , &corridorWallCorner);
	corridorWallCornerTex = (OGLTexture*)TextureLoader::LoadAPITexture("Corridor_Walls_Redux_Metal.png");
	loadFunc("Corridor_Wall_Light.msh"	 , &corridorWallLight);
	corridorWallLightTex = (OGLTexture*)TextureLoader::LoadAPITexture("checkerboard.png");
	loadFunc("Security_Camera.msh"	 , &securityCamera);
	securityCameraTex = (OGLTexture*)TextureLoader::LoadAPITexture("checkerboard.png");
	loadFunc("corridor_wall_screen.msh"	 , &corridorWallScreen);
	corridorWallScreenTex = (OGLTexture*)TextureLoader::LoadAPITexture("Animated_Screens_A_Colour.png");
	loadFunc("corridor_Wall_Straight_Mid_end_L.msh"	 , &corridorWallStraight);
	corridorWallStraightTex = (OGLTexture*)TextureLoader::LoadAPITexture("Corridor_Walls_Redux_Colour.png");
	loadFunc("Corridor_Wall_Hammer.msh"	 , &corridorWallHammer);
	corridorWallHammerTex = (OGLTexture*)TextureLoader::LoadAPITexture("checkerboard.png");

	basicTex	= (OGLTexture*)TextureLoader::LoadAPITexture("checkerboard.png");
	basicShader = new OGLShader("GameTechVert.glsl", "GameTechFrag.glsl");
	playerTex = (OGLTexture*)TextureLoader::LoadAPITexture("me.png");

	InitCamera();
	InitWorld();
}

TutorialGame::~TutorialGame()	{
	delete cubeMesh;
	delete sphereMesh;
	delete charMeshA;
	delete charMeshB;
	delete enemyMesh;
	delete bonusMesh;

	delete basicTex;
	delete basicShader;

	delete physics;
	delete renderer;
	delete world;
}

void TutorialGame::UpdateGame(float dt) {
	Debug::SetRenderer(renderer);
	switch (state) {
	case PLAY: UpdateGameWorld(dt); break;
	case PAUSE: UpdatePauseScreen(dt); break;
	case WIN: UpdateWinScreen(dt); break;
	case RESET: {
		InitCamera();
		InitWorld();
		selectionObject = nullptr;
		break;
	}
	}

	inputHandler->HandleInput();

	//Debug::DrawLine(Vector3(), Vector3(0, 20, 0), Debug::RED);
	//Debug::DrawLine(Vector3(), Vector3(360, 0, 0), Debug::RED);
	//Debug::DrawLine(Vector3(360, 0, 0), Vector3(360, 0, 360), Debug::RED);
	//Debug::DrawLine(Vector3(360, 0, 360), Vector3(0, 0, 360), Debug::RED);
	//Debug::DrawLine(Vector3(0, 0, 360), Vector3(0, 0, 0), Debug::RED);

	renderer->Update(dt);

	Debug::FlushRenderables(dt);
	renderer->Render();
}

void TutorialGame::UpdateGameWorld(float dt)
{
	if (!inSelectionMode) {
		world->GetMainCamera()->UpdateCamera(dt);
	}

	UpdateKeys();

	if (physics->GetGravity()) {
		Debug::Print("(G)ravity on", Vector2(5, 95));
	}
	else {
		Debug::Print("(G)ravity off", Vector2(5, 95));
	}

	if (debugDraw) {
		GameObjectIterator first;
		GameObjectIterator last;
		world->GetObjectIterators(first, last);
		for (auto i = first; i != last; i++) {
			DebugDrawCollider((*i)->GetBoundingVolume(), &(*i)->GetTransform());
			if ((*i)->GetPhysicsObject() == nullptr)
				continue;
			DebugDrawVelocity((*i)->GetPhysicsObject()->GetLinearVelocity(), &(*i)->GetTransform());
		}
	}

	SelectObject();
	MoveSelectedObject(dt);
	physics->Update(dt);

	world->UpdateWorld(dt);
}

void TutorialGame::DebugDrawCollider(const CollisionVolume* c, Transform* worldTransform) {
	Vector4 col = Vector4(1, 0, 0, 1);
	if (c == nullptr)
		return;
	switch (c->type) {
	case VolumeType::AABB: Debug::DrawCube(worldTransform->GetPosition(), ((AABBVolume*)c)->GetHalfDimensions(), col); break;
	case VolumeType::OBB: Debug::DrawCube(worldTransform->GetPosition(), ((AABBVolume*)c)->GetHalfDimensions(), Vector4(0, 1, 0, 1), 0, worldTransform->GetOrientation()); break;
	case VolumeType::Sphere: Debug::DrawSphere(worldTransform->GetPosition(), ((SphereVolume*)c)->GetRadius(), col); break;
	case VolumeType::Capsule: Debug::DrawCapsule(worldTransform->GetPosition(), ((CapsuleVolume*)c)->GetRadius(), ((CapsuleVolume*)c)->GetHalfHeight(), worldTransform->GetOrientation(), col); break;
	default: break;
	}
}

void TutorialGame::DebugDrawVelocity(const Vector3& velocity, Transform* worldTransform) {
	Vector4 col = Vector4(1, 0, 1, 1);
	Debug::DrawArrow(worldTransform->GetPosition(), worldTransform->GetPosition() + velocity, col);
}

void TutorialGame::DebugDrawObjectInfo(const GameObject* obj) {
	Vector3 pos = selectionObject->GetTransform().GetPosition();
	Vector3 rot = selectionObject->GetTransform().GetOrientation().ToEuler();
	string name = obj->GetName();
	string n = "Name: " + (name == "" ? "-" : name);
	string p = "Pos: (" + std::to_string(pos.x) + ", " + std::to_string(pos.y) + ", " + std::to_string(pos.z) + ")";
	string r = "Rot: (" + std::to_string(rot.x) + ", " + std::to_string(rot.y) + ", " + std::to_string(rot.z) + ")";
	renderer->DrawString(n, Vector2(1, 3), Debug::WHITE, 15.0f);
	renderer->DrawString(p, Vector2(1, 6), Debug::WHITE, 15.0f);
	renderer->DrawString(r, Vector2(1, 9), Debug::WHITE, 15.0f);
}

void TutorialGame::UpdatePauseScreen(float dt)
{
	renderer->DrawString("PAUSED", Vector2(5, 80), Debug::MAGENTA, 30.0f);
	renderer->DrawString("Press P to Unpause.", Vector2(5, 90), Debug::WHITE, 20.0f);
	renderer->DrawString("Press Esc to return to Main Menu.", Vector2(5, 95), Debug::WHITE, 20.0f);
}

void TutorialGame::UpdateWinScreen(float dt)
{
	renderer->DrawString("YOU WIN", Vector2(5, 80), Debug::MAGENTA, 30.0f);
	renderer->DrawString("Press R to Restart.", Vector2(5, 90), Debug::WHITE, 20.0f);
	renderer->DrawString("Press Esc to return to Main Menu.", Vector2(5, 95), Debug::WHITE, 20.0f);
}

void TutorialGame::UpdateKeys() {
	if (Window::GetKeyboard()->KeyPressed(KeyboardKeys::F1)) {
		InitWorld(); //We can reset the simulation at any time with F1
		selectionObject = nullptr;
	}

	if (Window::GetKeyboard()->KeyPressed(KeyboardKeys::F2)) {
		InitCamera(); //F2 will reset the camera to a specific default place
	}
	
	//Running certain physics updates in a consistent order might cause some
	//bias in the calculations - the same objects might keep 'winning' the constraint
	//allowing the other one to stretch too much etc. Shuffling the order so that it
	//is random every frame can help reduce such bias.
	if (Window::GetKeyboard()->KeyPressed(KeyboardKeys::F9)) {
		world->ShuffleConstraints(true);
	}
	if (Window::GetKeyboard()->KeyPressed(KeyboardKeys::F10)) {
		world->ShuffleConstraints(false);
	}

	if (Window::GetKeyboard()->KeyPressed(KeyboardKeys::F7)) {
		world->ShuffleObjects(true);
	}
	if (Window::GetKeyboard()->KeyPressed(KeyboardKeys::F8)) {
		world->ShuffleObjects(false);
	}

	DebugObjectMovement();
}

void TutorialGame::DebugObjectMovement() {
//If we've selected an object, we can manipulate it with some key presses
	if (inSelectionMode && selectionObject) {
		//Twist the selected object!
		//if (Window::GetKeyboard()->KeyDown(KeyboardKeys::LEFT)) {
		//	selectionObject->GetPhysicsObject()->AddTorque(Vector3(-10, 0, 0));
		//}

		//if (Window::GetKeyboard()->KeyDown(KeyboardKeys::RIGHT)) {
		//	selectionObject->GetPhysicsObject()->AddTorque(Vector3(10, 0, 0));
		//}

		if (Window::GetKeyboard()->KeyDown(KeyboardKeys::NUM7)) {
			selectionObject->GetPhysicsObject()->AddTorque(Vector3(0, 10, 0));
		}

		if (Window::GetKeyboard()->KeyDown(KeyboardKeys::NUM8)) {
			selectionObject->GetPhysicsObject()->AddTorque(Vector3(0, -10, 0));
		}

		//if (Window::GetKeyboard()->KeyDown(KeyboardKeys::RIGHT)) {
		//	selectionObject->GetPhysicsObject()->AddTorque(Vector3(10, 0, 0));
		//}

		//if (Window::GetKeyboard()->KeyDown(KeyboardKeys::UP)) {
		//	selectionObject->GetPhysicsObject()->AddForce(Vector3(0, 0, -10));
		//}

		//if (Window::GetKeyboard()->KeyDown(KeyboardKeys::DOWN)) {
		//	selectionObject->GetPhysicsObject()->AddForce(Vector3(0, 0, 10));
		//}

		if (Window::GetKeyboard()->KeyDown(KeyboardKeys::NUM5)) {
			selectionObject->GetPhysicsObject()->AddForce(Vector3(0, -10, 0));
		}
	}

}

void TutorialGame::InitCamera() {
	world->GetMainCamera()->SetNearPlane(0.1f);
	world->GetMainCamera()->SetFarPlane(500.0f);
	world->GetMainCamera()->SetPitch(-15.0f);
	world->GetMainCamera()->SetYaw(315.0f);
	world->GetMainCamera()->SetPosition(Vector3(-60, 40, 60));
}

void TutorialGame::InitWorld() {
	world->ClearAndErase();
	physics->Clear();

	GameObject* floor = AddFloorToWorld(Vector3(0, 0, 0));
	AddLongWallToWorld(Vector3(255,0,5), Vector3(2, 20, 250), 270, corridorWallStraight, corridorWallAlertTex);
	AddLongWallToWorld(Vector3(-255,0,5), Vector3(2, 20, 250), 90, corridorWallStraight, corridorWallAlertTex);
	AddLongWallToWorld(Vector3(5,0,255), Vector3(250, 20, 2), 180, corridorWallStraight, corridorWallAlertTex);
	AddLongWallToWorld(Vector3(5,0,-255), Vector3(250, 20, 2), 0, corridorWallStraight, corridorWallAlertTex);

	AddCornerWallToWorld(Vector3(-247.5, 0, -250), Vector3(8, 5, 4), 45);
	AddCornerWallToWorld(Vector3(-247.5, 0, 250), Vector3(8, 5, 4), 135);
	AddCornerWallToWorld(Vector3(247.5, 0, 250), Vector3(8, 5, 4), 225);
	AddCornerWallToWorld(Vector3(247.5, 0, -250), Vector3(8, 5, 4), 315);

	AddSecurityCameraToWorld(Vector3(25, 4, 240), 180);
	AddSecurityCameraToWorld(Vector3(-25, 4, 240), 180);

	/*AddWallHammerToWorld(Vector3(0, 2, 241), Vector3(10, 10, 6), 180);
	AddWallHammerToWorld(Vector3(50, 2, 241), Vector3(10, 10, 6), 180);
	AddWallHammerToWorld(Vector3(-50, 2, 241), Vector3(10, 10, 6), 180);*/

	//InitSphereGridWorld(10, 10, 25, 25, 2);

	Player* player = AddPlayerToWorld(Vector3(0, 5, 0));
	player->SetDynamic(true);

	Command* f = new MoveForwardCommand(player);
	Command* b = new MoveBackwardCommand(player);
	Command* l = new MoveLeftCommand(player);
	Command* r = new MoveRightCommand(player);
	inputHandler->BindButtonW(f);
	inputHandler->BindButtonS(b);
	inputHandler->BindButtonA(l);
	inputHandler->BindButtonD(r);


	GameObject* cap1 = AddCapsuleToWorld(Vector3(15, 5, 0), 3.0f, 1.5f);
	cap1->SetDynamic(true);

	cap1->SetCollisionLayers(CollisionLayer::LAYER_ONE | CollisionLayer::LAYER_TWO);
	player->SetCollisionLayers(CollisionLayer::LAYER_ONE);
	player1 = player;

	physics->BuildStaticList();
}

/*

A single function to add a large immoveable cube to the bottom of our world

*/
GameObject* TutorialGame::AddFloorToWorld(const Vector3& position) {
	GameObject* floor = new GameObject("Floor");

	Vector3 floorSize	= Vector3(250, 2, 250);
	AABBVolume* volume	= new AABBVolume(floorSize);
	floor->SetBoundingVolume((CollisionVolume*)volume);
	floor->GetTransform()
		.SetScale(floorSize * 2)
		.SetPosition(position);

	floor->SetRenderObject(new RenderObject(&floor->GetTransform(), cubeMesh, basicTex, basicShader));
	floor->SetPhysicsObject(new PhysicsObject(&floor->GetTransform(), floor->GetBoundingVolume()));

	floor->GetPhysicsObject()->SetInverseMass(0);
	floor->GetPhysicsObject()->InitCubeInertia();

	floor->SetCollisionLayers(CollisionLayer::LAYER_ONE);
	floor->SetDynamic(false);
	world->AddGameObject(floor);
	return floor;
}

/*

Builds a game object that uses a sphere mesh for its graphics, and a bounding sphere for its
rigid body representation. This and the cube function will let you build a lot of 'simple' 
physics worlds. You'll probably need another function for the creation of OBB cubes too.

*/
GameObject* TutorialGame::AddSphereToWorld(const Vector3& position, float radius, float inverseMass, bool rubber, bool hollow, bool dynamic) {
	GameObject* sphere = new GameObject("Sphere");
	
	Vector3 sphereSize = Vector3(radius, radius, radius);
	SphereVolume* volume = new SphereVolume(radius);
	sphere->SetBoundingVolume((CollisionVolume*)volume);

	sphere->GetTransform()
		.SetScale(sphereSize)
		.SetPosition(position);

	sphere->SetRenderObject(new RenderObject(&sphere->GetTransform(), sphereMesh, basicTex, basicShader));
	sphere->SetPhysicsObject(new PhysicsObject(&sphere->GetTransform(), sphere->GetBoundingVolume()));

	sphere->GetPhysicsObject()->SetInverseMass(inverseMass);
	if (hollow)
		sphere->GetPhysicsObject()->InitHollowSphereInertia();
	else
		sphere->GetPhysicsObject()->InitSphereInertia();

	if (rubber)
		sphere->GetPhysicsObject()->SetElasticity(0.9f);
	else
		sphere->GetPhysicsObject()->SetElasticity(0.2f);

	world->AddGameObject(sphere);
	sphere->SetDynamic(dynamic);
	return sphere;
}

GameObject* TutorialGame::AddCapsuleToWorld(const Vector3& position, float halfHeight, float radius, float inverseMass) {
	GameObject* capsule = new GameObject("Capsule");

	CapsuleVolume* volume = new CapsuleVolume(halfHeight, radius);
	capsule->SetBoundingVolume((CollisionVolume*)volume);

	capsule->GetTransform()
		.SetScale(Vector3(radius* 2, halfHeight, radius * 2))
		.SetPosition(position);

	capsule->SetRenderObject(new RenderObject(&capsule->GetTransform(), capsuleMesh, basicTex, basicShader));
	capsule->SetPhysicsObject(new PhysicsObject(&capsule->GetTransform(), capsule->GetBoundingVolume()));

	capsule->GetPhysicsObject()->SetInverseMass(inverseMass);
	capsule->GetPhysicsObject()->InitCubeInertia();

	world->AddGameObject(capsule);

	return capsule;
}

GameObject* TutorialGame::AddCubeToWorld(const Vector3& position, Vector3 dimensions, bool OBB, float inverseMass, int layer, bool isTrigger, bool dynamic) {
	GameObject* cube = new GameObject();
	if (OBB) {
		OBBVolume* volume = new OBBVolume(dimensions);
		cube->SetBoundingVolume((CollisionVolume*)volume);
	}
	else {
		AABBVolume* volume = new AABBVolume(dimensions);
		cube->SetBoundingVolume((CollisionVolume*)volume);
	}

	cube->GetTransform()
		.SetPosition(position)
		.SetScale(dimensions * 2);


	cube->SetRenderObject(new RenderObject(&cube->GetTransform(), cubeMesh, basicTex, basicShader));
	cube->SetPhysicsObject(new PhysicsObject(&cube->GetTransform(), cube->GetBoundingVolume()));

	cube->GetPhysicsObject()->SetInverseMass(inverseMass);
	cube->GetPhysicsObject()->InitCubeInertia();
	cube->GetPhysicsObject()->SetElasticity(0.2f);

	world->AddGameObject(cube);
	switch (layer)
	{
	default:
		cube->SetCollisionLayers(CollisionLayer::LAYER_ONE);
		break;
	case 1:
		cube->SetCollisionLayers(CollisionLayer::LAYER_ONE);
		break;
	case 2:
		cube->SetCollisionLayers(CollisionLayer::LAYER_TWO);
		break;
	case 3:
		cube->SetCollisionLayers(CollisionLayer::LAYER_THREE);
		break;
	}
	cube->SetTrigger(isTrigger);
	cube->SetDynamic(dynamic);
	return cube;
}

void TutorialGame::AddLongWallToWorld(const Vector3& position, Vector3 dimensions, int rotation, OGLMesh* mesh, OGLTexture* texture) {
	Vector3 location = position + Vector3(0, 10, 0);

	if (position.x < 0)
		location += Vector3(5, 0, -5);
	if (position.x > 0)
		location += Vector3(-7, 0, -4);

	if (position.z < 0)
		location += Vector3(1, 0, 10);
	if (position.z > 0)
		location += Vector3(1, 0, -2);

	GameObject* mainWall = AddAABBWallToWorld(location, dimensions, rotation);
	if (rotation == 90 || rotation == 270)
	{
		for (int i = -dimensions.z; i < dimensions.z; i += 10)
		{
			AddRenderPartToWorld(Vector3(position.x, position.y, position.z + i), Vector3(5, 5, 4), rotation, corridorWallStraight, corridorWallAlertTex);
		}
		return;
	}
	if (rotation == 180 || rotation == 0)
	{
		for (int i = -dimensions.x; i < dimensions.x; i += 10)
		{
			AddRenderPartToWorld(Vector3(position.x + i, position.y, position.z), Vector3(5, 5, 4), rotation, corridorWallStraight, corridorWallAlertTex);
		}
		return;
	}
	return;
}
GameObject* TutorialGame::AddAABBWallToWorld(const Vector3& position, Vector3 dimensions, int rotation) {
	GameObject* cube = new GameObject();
	AABBVolume* volume = new AABBVolume(dimensions);
	cube->SetBoundingVolume((CollisionVolume*)volume);

	cube->GetTransform()
		.SetPosition(position)
		.SetScale(dimensions * 2)
		.SetOrientation(Quaternion::EulerAnglesToQuaternion(0, rotation, 0));

	cube->SetRenderObject(nullptr);
	cube->SetPhysicsObject(new PhysicsObject(&cube->GetTransform(), cube->GetBoundingVolume()));

	cube->GetPhysicsObject()->SetInverseMass(0.0f);
	cube->GetPhysicsObject()->InitCubeInertia();

	cube->SetCollisionLayers(CollisionLayer::LAYER_ONE);
	cube->SetDynamic(false);
	world->AddGameObject(cube);
	return cube;
}
GameObject* TutorialGame::AddRenderPartToWorld(const Vector3& position, Vector3 dimensions, int rotation, OGLMesh* mesh, OGLTexture* texture) {
	GameObject* cube = new GameObject();
	cube->SetBoundingVolume(nullptr);
	
	cube->GetTransform()
		.SetPosition(position)
		.SetScale(dimensions * 2)
		.SetOrientation(Quaternion::EulerAnglesToQuaternion(0, rotation, 0));

	cube->SetRenderObject(new RenderObject(&cube->GetTransform(), mesh, texture, basicShader));
	cube->SetPhysicsObject(nullptr);

	cube->SetDynamic(false);
	world->AddGameObject(cube);
	return cube;
}

GameObject* TutorialGame::AddWallToWorld(const Vector3& position, Vector3 dimensions, int rotation) {
	GameObject* cube = new GameObject();
	
	AABBVolume* volume = new AABBVolume(dimensions);
	cube->SetBoundingVolume((CollisionVolume*)volume);

	cube->GetTransform()
		.SetPosition(position)
		.SetScale(dimensions * 2)
		.SetOrientation(Quaternion::EulerAnglesToQuaternion(0, rotation, 0));

	cube->SetRenderObject(new RenderObject(&cube->GetTransform(), corridorWallStraight, corridorWallAlertTex, basicShader));
	cube->SetPhysicsObject(new PhysicsObject(&cube->GetTransform(), cube->GetBoundingVolume()));

	cube->GetPhysicsObject()->SetInverseMass(0.0f);
	cube->GetPhysicsObject()->InitCubeInertia();

	cube->SetCollisionLayers(CollisionLayer::LAYER_ONE);
	cube->SetDynamic(false);
	world->AddGameObject(cube);
	return cube;
}
GameObject* TutorialGame::AddOBBWallToWorld(const Vector3& position, Vector3 dimensions, int rotation) {
	GameObject* cube = new GameObject();
	
	OBBVolume* volume = new OBBVolume(dimensions + Vector3(2,10,0));
	cube->SetBoundingVolume((CollisionVolume*)volume);

	cube->GetTransform()
		.SetPosition(position)
		.SetScale(dimensions * 2)
		.SetOrientation(Quaternion::EulerAnglesToQuaternion(0, rotation, 0));

	cube->SetRenderObject(nullptr);
	cube->SetPhysicsObject(new PhysicsObject(&cube->GetTransform(), cube->GetBoundingVolume()));

	cube->GetPhysicsObject()->SetInverseMass(0.0f);
	cube->GetPhysicsObject()->InitCubeInertia();

	cube->SetCollisionLayers(CollisionLayer::LAYER_ONE);
	cube->SetDynamic(false);
	world->AddGameObject(cube);
	return cube;
}
void TutorialGame::AddCornerWallToWorld(const Vector3& position, Vector3 dimensions, int rotation) {
	Vector3 location = position + Vector3(0, 15, 0);

	if (rotation == 45)
		location += Vector3(3, 0, 3);
	if (rotation == 135)
		location += Vector3(3, 0, -3);
	if (rotation == 225)
		location += Vector3(-3, 0, -3);
	if (rotation == 315)
		location += Vector3(-3, 0, 3);

	GameObject* mainWall = AddOBBWallToWorld(location, dimensions, rotation);
	AddRenderPartToWorld(position, dimensions, rotation, corridorWallCorner, corridorWallAlertTex);
	return;
}
void TutorialGame::AddSecurityCameraToWorld(const Vector3& position, int rotation)
{
	Vector3 location = position + Vector3(0, 24, 0);
	Vector3 dimensions = Vector3(2, 3, 4);
	if (rotation == 0)
	{
		dimensions = Vector3(2, 2, 4);
		location += Vector3(0, 0, -4);
	}
	if (rotation == 90)
	{
		dimensions = Vector3(4, 2, 2);
		location += Vector3(-4, 0, 0);
	}
	if (rotation == 180)
	{
		dimensions = Vector3(2, 2, 4);
		location += Vector3(0, 0, 4);
	}
	if (rotation == 270)
	{
		dimensions = Vector3(4, 2, 2);
		location += Vector3(4, 0, 0);
	}

	GameObject* mainWall = AddAABBWallToWorld(location, dimensions, rotation);
	AddRenderPartToWorld(position, Vector3(5, 5, 5), rotation, securityCamera, securityCameraTex);
	return;
}
void TutorialGame::AddWallHammerToWorld(const Vector3& position, Vector3 dimensions, int rotation)
{
	GameObject* cube = new GameObject();

	AABBVolume* volume = new AABBVolume(dimensions);
	cube->SetBoundingVolume((CollisionVolume*)volume);

	cube->GetTransform()
		.SetPosition(position)
		.SetScale(dimensions * 2)
		.SetOrientation(Quaternion::EulerAnglesToQuaternion(0, rotation, 0));


	cube->SetRenderObject(new RenderObject(&cube->GetTransform(), corridorWallHammer, corridorWallHammerTex, basicShader));
	cube->SetPhysicsObject(new PhysicsObject(&cube->GetTransform(), cube->GetBoundingVolume()));

	cube->GetPhysicsObject()->SetInverseMass(0.0f);
	cube->GetPhysicsObject()->InitCubeInertia();

	cube->SetCollisionLayers(CollisionLayer::LAYER_ONE);
	cube->SetDynamic(false);
	world->AddGameObject(cube);
	return;
}

void TutorialGame::InitSphereGridWorld(int numRows, int numCols, float rowSpacing, float colSpacing, float radius) {
	for (int x = 0; x < numCols; ++x) {
		for (int z = 0; z < numRows; ++z) {
			Vector3 position = Vector3(x * colSpacing, 10.0f, z * rowSpacing);
			GameObject* sphere = AddSphereToWorld(position, radius, 10.0f, false, false, false);
			sphere->SetCollisionLayers(CollisionLayer::LAYER_ONE);
		}
	}
}

void TutorialGame::InitCapsuleGridWorld(int numRows, int numCols, float rowSpacing, float colSpacing) {
	for (int x = 0; x < numCols; ++x) {
		for (int z = 0; z < numRows; ++z) {
			Vector3 position = Vector3(x * colSpacing, 10.0f, z * rowSpacing);
			GameObject* capsule = AddCapsuleToWorld(position, 3.0f, 1.5f);
			capsule->SetDynamic(true);
			capsule->SetCollisionLayers(CollisionLayer::LAYER_ONE);
		}
	}
}

void TutorialGame::InitMixedGridWorld(int numRows, int numCols, float rowSpacing, float colSpacing) {
	float sphereRadius = 1.0f;
	Vector3 cubeDims = Vector3(1, 1, 1);

	for (int x = 0; x < numCols; ++x) {
		for (int z = 0; z < numRows; ++z) {
			Vector3 position = Vector3(x * colSpacing, 10.0f, z * rowSpacing);
			int choice = rand() % 3;
			if (choice == 2) {
				GameObject* obb = AddCubeToWorld(position, cubeDims, true, 10.0f, 1, false, true);
				obb->SetCollisionLayers(CollisionLayer::LAYER_ONE);
			}
			else if (choice == 1) {
				GameObject* capsule = AddCapsuleToWorld(position, 3.0f, 1.5f);
				capsule->SetDynamic(true);
				capsule->SetCollisionLayers(CollisionLayer::LAYER_ONE);
			}
			else {
				GameObject* sphere = AddSphereToWorld(position, sphereRadius, 10.0f, false, false, true);
				sphere->SetCollisionLayers(CollisionLayer::LAYER_ONE);
			}
		}
	}
}

void TutorialGame::InitCubeGridWorld(int numRows, int numCols, float rowSpacing, float colSpacing, const Vector3& cubeDims) {
	for (int x = 1; x < numCols+1; ++x) {
		for (int z = 1; z < numRows+1; ++z) {
			Vector3 position = Vector3(x * colSpacing, 10.0f, z * rowSpacing);
			GameObject* obb = AddCubeToWorld(position, cubeDims, true, 10.0f, 1, false, true);
			obb->SetCollisionLayers(CollisionLayer::LAYER_ONE);
		}
	}
}

void TutorialGame::InitDefaultFloor() {
	AddFloorToWorld(Vector3(0, -2, 0));
}

void TutorialGame::InitGameExamples() {
	AddPlayerToWorld(Vector3(0, 5, 0));
	AddCapsuleToWorld(Vector3(15, 5, 0), 3.0f, 1.5f, 1.0f);
}

Player* TutorialGame::AddPlayerToWorld(const Vector3& position) {
	float meshSize = 3.0f;
	float inverseMass = 0.5f;

	Player* character = new Player(world->GetMainCamera(), "Player");

	AABBVolume* volume = new AABBVolume(Vector3(0.3f, 0.85f, 0.3f) * meshSize);

	character->SetBoundingVolume((CollisionVolume*)volume);

	character->GetTransform()
		.SetScale(Vector3(meshSize, meshSize, meshSize))
		.SetPosition(position);

	character->SetRenderObject(new RenderObject(&character->GetTransform(), charMeshA, nullptr, basicShader));
	character->SetPhysicsObject(new PhysicsObject(&character->GetTransform(), character->GetBoundingVolume()));

	character->GetPhysicsObject()->SetInverseMass(inverseMass);
	character->GetPhysicsObject()->SetFriction(0.0f);
	character->GetPhysicsObject()->InitSphereInertia();

	world->AddGameObject(character);

	return character;
}

/*

Every frame, this code will let you perform a raycast, to see if there's an object
underneath the cursor, and if so 'select it' into a pointer, so that it can be 
manipulated later. Pressing Q will let you toggle between this behaviour and instead
letting you move the camera around. 

*/
bool TutorialGame::SelectObject() {
	if (Window::GetKeyboard()->KeyPressed(KeyboardKeys::Q)) {
		inSelectionMode = !inSelectionMode;
		if (inSelectionMode) {
			Window::GetWindow()->ShowOSPointer(true);
			Window::GetWindow()->LockMouseToWindow(false);
		}
		else {
			Window::GetWindow()->ShowOSPointer(false);
			Window::GetWindow()->LockMouseToWindow(true);
		}
	}
	if (inSelectionMode) {
		renderer->DrawString("Press Q to change to camera mode!", Vector2(5, 85));

		if (Window::GetMouse()->ButtonDown(NCL::MouseButtons::LEFT)) {
			if (selectionObject) {	//set colour to deselected;
				if(selectionObject->GetRenderObject())
					selectionObject->GetRenderObject()->SetColour(Vector4(1, 1, 1, 1));
				//selectionObject->SetLayer(0);
				selectionObject = nullptr;
			}

			Ray ray = CollisionDetection::BuildRayFromMouse(*world->GetMainCamera());

			RayCollision closestCollision;
			if (world->Raycast(ray, closestCollision, true)) {
				selectionObject = (GameObject*)closestCollision.node;
				if(selectionObject->GetRenderObject())
					selectionObject->GetRenderObject()->SetColour(Vector4(0, 1, 0, 1));
				//selectionObject->SetLayer(1);
				//Ray r(selectionObject->GetTransform().GetPosition(), Vector3(0,0,-1));
				//RayCollision col;

				// Doesn't work for cubes for some reason idk

				//if (world->Raycast(r, col, true)) {
				//	((GameObject*)col.node)->GetRenderObject()->SetColour(Vector4(0, 0, 1, 1));
				//	Debug::DrawLine(r.GetPosition(), col.collidedAt, Vector4(1, 0, 0, 1), 5.0f);
				//}

				Debug::DrawLine(ray.GetPosition(), closestCollision.collidedAt, Vector4(1, 0, 0, 1), 5.0f);
				return true;
			}
			else {
				return false;
			}
		}
	}
	else {
		renderer->DrawString("Press Q to change to select mode!", Vector2(5, 85));
	}

	if (Window::GetKeyboard()->KeyPressed(NCL::KeyboardKeys::L)) {
		player1->ChangeCamLock();
	}

	return false;
}

/*
If an object has been clicked, it can be pushed with the right mouse button, by an amount
determined by the scroll wheel. In the first tutorial this won't do anything, as we haven't
added linear motion into our physics system. After the second tutorial, objects will move in a straight
line - after the third, they'll be able to twist under torque aswell.
*/
void TutorialGame::MoveSelectedObject(float dt) {
	renderer->DrawString("Click Force: " + std::to_string(forceMagnitude), Vector2(10, 20));
	forceMagnitude += Window::GetMouse()->GetWheelMovement() * 100.0f;

	if (!selectionObject)
		return;

	if (Window::GetMouse()->ButtonPressed(NCL::MouseButtons::RIGHT)) {
		Ray ray = CollisionDetection::BuildRayFromMouse(*world->GetMainCamera());
		RayCollision closestCollision;
		if (world->Raycast(ray, closestCollision, true)) {
			if (closestCollision.node == selectionObject) {
				selectionObject->GetPhysicsObject()->AddForceAtPosition(ray.GetDirection() * forceMagnitude, closestCollision.collidedAt);
			}
		}
	}

	DebugDrawObjectInfo(selectionObject);

	if (Window::GetKeyboard()->KeyHeld(NCL::KeyboardKeys::F))
		selectionObject->Interact(dt);

	/*if (Window::GetKeyboard()->KeyDown(NCL::KeyboardKeys::UP)) {
		if (selectionObject->GetName() == "player")
			selectionObject->GetPhysicsObject()->AddForce(Vector3(0, 0, -1) * ((Player*)selectionObject)->GetSpeed() * 0.1);
		else
			selectionObject->GetPhysicsObject()->AddForce(Vector3(0, 0, -1) * forceMagnitude * 0.1);
	}
	if (Window::GetKeyboard()->KeyDown(NCL::KeyboardKeys::DOWN)) {
		if (selectionObject->GetName() == "player")
			selectionObject->GetPhysicsObject()->AddForce(Vector3(0, 0, 1) * ((Player*)selectionObject)->GetSpeed() * 0.1);
		else
			selectionObject->GetPhysicsObject()->AddForce(Vector3(0, 0, 1) * forceMagnitude * 0.1);
	}
	if (Window::GetKeyboard()->KeyDown(NCL::KeyboardKeys::LEFT)) {
		if (selectionObject->GetName() == "player")
			selectionObject->GetPhysicsObject()->AddForce(Vector3(-1, 0, 0) * ((Player*)selectionObject)->GetSpeed() * 0.1);
		else
			selectionObject->GetPhysicsObject()->AddForce(Vector3(-1, 0, 0) * forceMagnitude * 0.1);
	}
	if (Window::GetKeyboard()->KeyDown(NCL::KeyboardKeys::RIGHT)) {
		if (selectionObject->GetName() == "player")
			selectionObject->GetPhysicsObject()->AddForce(Vector3(1, 0, 0) * ((Player*)selectionObject)->GetSpeed() * 0.1);
		else
			selectionObject->GetPhysicsObject()->AddForce(Vector3(1, 0, 0) * forceMagnitude * 0.1);
	}
	if (Window::GetKeyboard()->KeyDown(NCL::KeyboardKeys::SHIFT)) {
		if (selectionObject->GetName() == "player")
			selectionObject->GetPhysicsObject()->AddForce(Vector3(0, -1, 0) * ((Player*)selectionObject)->GetSpeed() * 0.1);
		else
			selectionObject->GetPhysicsObject()->AddForce(Vector3(0, -1, 0) * forceMagnitude * 0.1);
	}
	if (Window::GetKeyboard()->KeyDown(NCL::KeyboardKeys::SPACE)) {
		if (selectionObject->GetName() == "player")
			selectionObject->GetPhysicsObject()->AddForce(Vector3(0, 1, 0) * ((Player*)selectionObject)->GetSpeed() * 0.1);
		else
			selectionObject->GetPhysicsObject()->AddForce(Vector3(0, 1, 0) * forceMagnitude * 0.1);
	}*/
}