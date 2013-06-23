#if SAC_INGAME_EDITORS

#include "RecursiveRunnerDebugConsole.h"

#include "base/Log.h"
#include "base/ObjectSerializer.h"

#include "util/ScoreStorageProxy.h"

#include <glm/gtc/random.hpp>

#include "RecursiveRunnerGame.h"
#include "api/StorageAPI.h"

#include "base/StateMachine.h"

RecursiveRunnerGame* RecursiveRunnerDebugConsole::_game = 0;

void RecursiveRunnerDebugConsole::callbackFinishGame(void* arg) {
    _game->sceneStateMachine.overrideNextState = *((Scene::Enum*)arg);
    _game->sceneStateMachine.override = true;
}

void RecursiveRunnerDebugConsole::init(RecursiveRunnerGame* game) {
    _game = game;

    TwEnumVal arrivalScene[] = {
        { Scene::Menu, "Menu" },
        { Scene::Game, "Game" },
        { Scene::Tutorial, "Tutorial" },
        { Scene::Pause, "Pause" },
        { Scene::Ad, "Ad" },
        { Scene::Rate, "Rate" },
        { Scene::RestartGame, "RestartGame" },
     };

    REGISTER_ONE_ARG(FinishGame, arrivalScene)

/*
    static auto lambda = [game](void* arg) {
        LOGE("called!")
        game->sceneStateMachine.overrideNextState = Scene::Game;// *((Scene::Enum*)arg);
        game->sceneStateMachine.override = true;
    };
    TwAddButton(DebugConsole::Instance().bar, "test", (TwButtonCallback)&lambda, 0, "");
*/
}
#endif
