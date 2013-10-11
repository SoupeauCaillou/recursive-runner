#include "SuccessManager.h"
#include "RecursiveRunnerGame.h"

#include <systems/SessionSystem.h>
#include <systems/RunnerSystem.h>
#include <systems/PlayerSystem.h>

#include <api/StorageAPI.h>

#include <base/ObjectSerializer.h>

void SuccessManager::init(RecursiveRunnerGame* g) {
    game = g;
    
    //initialize achievement progression if not initialized yet
    game->gameThreadContext->storageAPI->setOption("Achievement0Step", std::string(), "0");
    game->gameThreadContext->storageAPI->setOption("Achievement4Step", std::string(), "0");

    achievement0CurrentStep = ObjectSerializer<int>::string2object(game->gameThreadContext->storageAPI->getOption("Achievement0Step"));
    achievement4CurrentStep = ObjectSerializer<int>::string2object(game->gameThreadContext->storageAPI->getOption("Achievement4Step"));
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
        game->gameThreadContext->gameCenterAPI->updateAchievementProgression(0, bestLighter - achievement0CurrentStep);
        achievement0CurrentStep = bestLighter;

        auto v = ObjectSerializer<int>::object2string(achievement0CurrentStep);
        game->gameThreadContext->storageAPI->setOption("Achievement0Step", v, v);
    }

    // open 7/20 lights with each runner
    if (totalGoodLighters > achievement4CurrentStep) {
        game->gameThreadContext->gameCenterAPI->updateAchievementProgression(4, totalGoodLighters - achievement4CurrentStep);
        achievement4CurrentStep = totalGoodLighters;

        auto v = ObjectSerializer<int>::object2string(achievement4CurrentStep);
        game->gameThreadContext->storageAPI->setOption("Achievement4Step", v, v);
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
