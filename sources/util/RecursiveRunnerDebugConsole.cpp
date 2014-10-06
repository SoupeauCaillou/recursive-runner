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
