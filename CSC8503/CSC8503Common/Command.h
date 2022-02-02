#pragma once
#include "GameActor.h"
#include "PhysicsSystem.h"
#include "../GameTech/TutorialGame.h"

namespace NCL {
	namespace CSC8503 {
		class Command {
		public:
			virtual ~Command() {};
			virtual void execute() = 0;
		};

#pragma region Actor Commands

		class MoveForwardCommand : public Command {
		public:
			MoveForwardCommand(GameActor* actor) : actor(actor) {};
			virtual ~MoveForwardCommand() {};
			void execute() {
				actor->MoveForwards();
			}

		protected:
			GameActor* actor;
		};

		class MoveBackwardCommand : public Command {
		public:
			MoveBackwardCommand(GameActor* actor) : actor(actor) {};
			virtual ~MoveBackwardCommand() {};
			void execute() {
				actor->MoveBackwards();
			}

		protected:
			GameActor* actor;
		};

		class MoveLeftCommand : public Command {
		public:
			MoveLeftCommand(GameActor* actor) : actor(actor) {};
			virtual ~MoveLeftCommand() {};
			void execute() {
				actor->MoveLeft();
			}

		protected:
			GameActor* actor;
		};

		class MoveRightCommand : public Command {
		public:
			MoveRightCommand(GameActor* actor) : actor(actor) {};
			virtual ~MoveRightCommand() {};
			void execute() {
				actor->MoveRight();
			}

		protected:
			GameActor* actor;
		};

#pragma endregion

#pragma region World Commands
		class ToggleGravityCommand : public Command {
		public:
			ToggleGravityCommand(PhysicsSystem* physics) : physics(physics) {};
			virtual ~ToggleGravityCommand() {};
			void execute() {
				if (physics->GetGravity())
					physics->UseGravity(false);
				else
					physics->UseGravity(true);
			}
		protected:
			PhysicsSystem* physics;
		};

		class ToggleDebugCommand : public Command {
		public:
			ToggleDebugCommand(TutorialGame* game) : game(game) {};
			virtual ~ToggleDebugCommand() {};
			void execute() {
				if (game->GetDebugDrawing())
					game->SetDebugDrawing(false);
				else
					game->SetDebugDrawing(true);
			}
		protected:
			TutorialGame* game;
		};
#pragma endregion
	}
}


