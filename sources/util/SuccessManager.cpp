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
#include "SuccessManager.h"
#include "RecursiveRunnerGame.h"

#include <systems/SessionSystem.h>
#include <systems/RunnerSystem.h>
#include <systems/PlayerSystem.h>

#include <api/StorageAPI.h>

#include <base/ObjectSerializer.h>

void SuccessManager::init(RecursiveRunnerGame* g) {
    game = g;

    achievement0CurrentStep = 0;
    achievement4CurrentStep = 0;
}

void SuccessManager::gameStart(bool bIsTuto) {
    isTuto = bIsTuto;

    totalLiving = maxLiving = 1;
    bestLighter = totalGoodLighters = 0;
}

void SuccessManager::oneMoreRunner(int scoreLastRunner) {
    if (isTuto) {
        LOGW_EVERY_N(120, "This is the tuto! Do not unlock successes there...");
        return;
    }

    ++totalLiving;

    LOGV(1, "One more: " << totalLiving << " previous score: " << scoreLastRunner << ", best: "
     << bestLighter << ", good: " << totalGoodLighters);

    bestLighter = std::max(bestLighter, scoreLastRunner);
    maxLiving = std::max(maxLiving, totalLiving);

    if (scoreLastRunner >= 7) {
        ++totalGoodLighters;
    }
}
void SuccessManager::oneLessRunner() {
    if (isTuto) {
        LOGW_EVERY_N(120, "This is the tuto! Do not unlock successes there...");
        return;
    }
    --totalLiving;
    LOGV(1, "One less: " << totalLiving);
}

#if SAC_USE_PROPRIETARY_PLUGINS
void SuccessManager::gameEnd(SessionComponent* sc) {
    if (isTuto) {
        LOGW_EVERY_N(120, "This is the tuto! Do not unlock successes there...");
        return;
    }

    LOGV(1, "End Game: " << maxLiving << " " << bestLighter << " " << totalGoodLighters);

    // have N C {4,6,8} living runners
    if (maxLiving >= 4) {
        game->gameThreadContext->gameCenterAPI->unlockAchievement(1);
    }
    if (maxLiving >= 6) {
        game->gameThreadContext->gameCenterAPI->unlockAchievement(2);
    }
    if (maxLiving >= 8) {
        game->gameThreadContext->gameCenterAPI->unlockAchievement(3);
    }

    // switch all the 20 lights in a row
    if (bestLighter > achievement0CurrentStep) {
        game->gameThreadContext->gameCenterAPI->updateAchievementProgression(0, bestLighter);
        achievement0CurrentStep = bestLighter;
    }

    // open 7/20 lights with each runner
    if (totalGoodLighters > achievement4CurrentStep) {
        game->gameThreadContext->gameCenterAPI->updateAchievementProgression(4, totalGoodLighters);
        achievement4CurrentStep = totalGoodLighters;
    }

    long finalScore = PLAYER(sc->players[0])->points;
    // reach a total score of {50K, 100K, 200K}
    if (finalScore >= 50000) {
        game->gameThreadContext->gameCenterAPI->unlockAchievement(5);
    }
    if (finalScore >= 100000) {
        game->gameThreadContext->gameCenterAPI->unlockAchievement(6);
    }
    if (finalScore >= 200000) {
        game->gameThreadContext->gameCenterAPI->unlockAchievement(7);
    }
}
#else
void SuccessManager::gameEnd(SessionComponent*) {}
#endif
