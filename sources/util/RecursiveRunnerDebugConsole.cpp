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

/*void RecursiveRunnerDebugConsole::callbackSubmitRandomScore(void* arg) {
    int count = *(int*)arg;*/

void RecursiveRunnerDebugConsole::callbackFinishGame(void*) {
    _game->sceneStateMachine.overrideNextState = Scene::Menu;
    _game->sceneStateMachine.override = true;
}

void RecursiveRunnerDebugConsole::init(RecursiveRunnerGame* game) {
    _game = game;

    //TwEnumVal numberToGenerate[] = { {1, "1"}, {10, "10"}, {100, "100"} };
    //REGISTER(SubmitRandomScore, numberToGenerate)

    REGISTER_NO_ARG(FinishGame)
}
#endif
