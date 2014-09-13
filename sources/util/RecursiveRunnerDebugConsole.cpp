#if SAC_INGAME_EDITORS && SAC_DEBUG

#include "RecursiveRunnerDebugConsole.h"

#include "RecursiveRunnerGame.h"

#include "systems/PlayerSystem.h"

RecursiveRunnerGame* RecursiveRunnerDebugConsole::game = 0;

void RecursiveRunnerDebugConsole::init(RecursiveRunnerGame* g) {
    game = g;
#if 0
    static float score = 10.f;
    DebugConsole::RegisterMethod("Force end game", callbackForceEndGame, "Score multiplier",
        TW_TYPE_FLOAT, &score);
#endif
}

void RecursiveRunnerDebugConsole::callbackForceEndGame(void* arg) {
    if (game->sceneStateMachine.getCurrentState() != Scene::Game) {
        LOGE("You are not playing! Abort");
        return;
    }
    float multiplier = *(float*)arg;

    for (auto player : thePlayerSystem.RetrieveAllEntityWithComponent()) {
        PLAYER(player)->points *= multiplier;
        LOGI("Player " << player << " score is now " << PLAYER(player)->points);
    }

    game->sceneStateMachine.forceNewState(Scene::Menu);
}
#endif
