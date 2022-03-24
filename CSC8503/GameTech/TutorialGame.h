#pragma once
#include "LevelLoader.h"
#include "../CSC8503Common/PhysicsSystem.h"
#include "../CSC8503Common/Player.h"
#include "../CSC8503Common/AudioManager.h"
#include "../CSC8503Common/BGMManager.h"
#include "../CSC8503Common/GameManager.h"
//#include "../CSC8503Common/Projectile.h"

namespace NCL {
	namespace PS4 {
		class InputBase;
	}
	namespace CSC8503 {
		enum class GameState {
			PLAY,
			PAUSE,
			WIN,
			RESET
		};

		class StateGameObject;
		class InputHandler;
		class Checkpoint;
		class TutorialGame		{
		public:
			TutorialGame();
			virtual ~TutorialGame();

			virtual void UpdateGame(float dt);

			void SetState(GameState s) { 
				state = s; 
				UpdateBGM(); 
			}

			bool Win() const { 
				return won;
			}

			bool GetPaused() {
				return pause;
			}

			bool GetQuit() {
				return quit;
			}

			void PaintObject();

		protected:
			InputHandler* inputHandler;

			void InitialiseAssets();

			void InitCamera();


			virtual void InitWorld();

			bool SelectObject();
			void MoveSelectedObject(float dt);
			void DebugDrawCollider(const CollisionVolume* c, Transform* worldTransform);
			void DebugDrawVelocity(const Vector3& vel, Transform* worldTransform);
			void DebugDrawObjectInfo(const GameObject* obj);
			void UpdateBGM();
			void UpdateScores(float dt);
			virtual void UpdatePauseState(float dt);

			int currentObj;

			float timeSinceLastScoreUpdate;

			Renderer*			renderer;
			PhysicsSystem*		physics;
			GameWorld*			world;
			NCL::AudioManager*	audio;
			BGMManager*			bgm;
			LevelLoader*		levelLoader;

			GameState state;
			GameManager* gameManager;

			void UpdateGameWorld(float dt);
			virtual void UpdatePauseScreen(float dt);
			void UpdateWinScreen(float dt);

			bool useGravity;
			bool inSelectionMode;
			bool debugDraw;
			bool pause = false;
			bool pausePressed = false;
			bool quit = false;

			float		forceMagnitude;

			GameObject* selectionObject = nullptr;

			bool won = false;
			Player* player1 = nullptr;
		};
	}
}

