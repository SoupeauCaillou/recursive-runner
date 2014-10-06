/*
    This file is part of RecursiveRunner.

    @author Soupe au Caillou - Jordane Pelloux-Prayer
    @author Soupe au Caillou - Gautier Pelloux-Prayer
    @author Soupe au Caillou - Pierre-Eric Pelloux-Prayer

    RecursiveRunner is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, version 3.

    RecursiveRunner is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with RecursiveRunner.  If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once
#include <base/Game.h>
#include <base/StateMachine.h>
class RecursiveRunnerGame;
namespace Scene {
	enum Enum : int {
		About,
		Game,
		Logo,
		Menu,
		Pause,
		Rate,
		RestartGame,
		Tutorial,
	};
	StateHandler<Scene::Enum>* CreateAboutSceneHandler(RecursiveRunnerGame* game);
	StateHandler<Scene::Enum>* CreateGameSceneHandler(RecursiveRunnerGame* game);
	StateHandler<Scene::Enum>* CreateLogoSceneHandler(RecursiveRunnerGame* game);
	StateHandler<Scene::Enum>* CreateMenuSceneHandler(RecursiveRunnerGame* game);
	StateHandler<Scene::Enum>* CreatePauseSceneHandler(RecursiveRunnerGame* game);
	StateHandler<Scene::Enum>* CreateRateSceneHandler(RecursiveRunnerGame* game);
	StateHandler<Scene::Enum>* CreateRestartGameSceneHandler(RecursiveRunnerGame* game);
	StateHandler<Scene::Enum>* CreateTutorialSceneHandler(RecursiveRunnerGame* game);
}
inline void registerScenes(RecursiveRunnerGame * game, StateMachine<Scene::Enum> & machine) {	machine.registerState(Scene::About, Scene::CreateAboutSceneHandler(game));
	machine.registerState(Scene::Game, Scene::CreateGameSceneHandler(game));
	machine.registerState(Scene::Logo, Scene::CreateLogoSceneHandler(game));
	machine.registerState(Scene::Menu, Scene::CreateMenuSceneHandler(game));
	machine.registerState(Scene::Pause, Scene::CreatePauseSceneHandler(game));
	machine.registerState(Scene::Rate, Scene::CreateRateSceneHandler(game));
	machine.registerState(Scene::RestartGame, Scene::CreateRestartGameSceneHandler(game));
	machine.registerState(Scene::Tutorial, Scene::CreateTutorialSceneHandler(game));
}