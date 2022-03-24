#include "MainMenu.h"
#include "../CSC8503Common/InputHandler.h"
using namespace NCL;
using namespace CSC8503;

MainMenu::MainMenu(TutorialGame* start, TutorialGame* training) 
	: renderer(RendererBase()), networkedLevel(start), trainingLevel(training) {
	pushMachine = new PushdownMachine((PushdownState*)this);
	inputHandler = new InputHandler();

	AxisCommand* m = new MenuMoveCommand(this);
	inputHandler->BindAxis(0, m);
	Command* enter = new MenuEnterCommand(this);
	inputHandler->BindButton(Input::JUMP, enter);
}

MainMenu::~MainMenu() {
	delete pushMachine;
	delete inputHandler;
}

	UpdateMenu(dt);

		break;
	case 0:
		*newState = new LevelState(networkedLevel);
		pressed = false;
		return PushdownState::PushdownResult::Push;
		break;
	case 1:
		*newState = new LevelState(trainingLevel);
		pressed = false;
		return PushdownState::PushdownResult::Push;
		break;
	case 2:
		return PushdownState::PushdownResult::Exit;
		break;
	}
}

void MainMenu::UpdateMenu(float dt) {
	renderer.Render();

	float framed = (renderer.GetFrameNumber() / 240.f);
	float size = (50.0f * abs(sin(framed))) + 40.0f;

	if (renderer.GetFrameNumber() % 5 == 1) {
		inputHandler->HandleInput();
	}

	renderer.DrawString("Spitoon", { 20,30 }, { 0.6f,0.3f,0.8f,1.0f },  size );

	auto drawMenuOption = [=](const std::string& string, const Maths::Vector2 pos, int selection, int menuNumber) {
		if (selection == menuNumber) {
			renderer.DrawString(string, pos, { 1.0f,0.2f,0.2f,1.0f });
		}
		else {
			renderer.DrawString(string, pos, { 0.8f,0.8f,0.8f,1.0f });
		}
	};

	drawMenuOption("Start Networked Game", { 30, 50 }, selectedItem, 0);
	drawMenuOption("Start Training Game", { 30, 60 }, selectedItem, 1);
	drawMenuOption("Quit", { 30, 70 }, selectedItem, 2);
}

void MainMenu::HandleMenuMove(const Vector2 axis) {
	if (axis.y > 0.1f) {
		selectedItem += 1;
	}
	if (axis.y < -0.1f) {
		selectedItem -= 1;
	}
	selectedItem = std::clamp(selectedItem, 0, 2);
}

void MainMenu::HandleMenuPress() {
	pressed = true;
}

bool MainMenu::UpdateGame(float dt) {
	bool val = false;
	!pushMachine->Update(dt) ? val = false : val =  true;
	return val;
}