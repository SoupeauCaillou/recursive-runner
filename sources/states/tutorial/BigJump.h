#pragma once

struct BigJumpTutorialStep : public TutorialStep {
    void enter(SessionComponent* , TutorialEntities* entities) {
        TEXT_RENDERING(entities->text)->hide = true;
        TEXT_RENDERING(entities->text)->text = "Do longer press to make higher jumps";
    }
    bool mustUpdateGame(SessionComponent* sc, TutorialEntities* ) {
        if (TRANSFORM(sc->currentRunner)->position.X < 0) {
            return true;
        } else {
            ANIMATION(sc->currentRunner)->playbackSpeed = 0;
            return false;
        }
    }
    bool canExit(SessionComponent* sc, TutorialEntities* entities) {
        if (TRANSFORM(sc->currentRunner)->position.X >= 0) {
            TEXT_RENDERING(entities->text)->hide = false;
            if (theTouchInputManager.wasTouched(0) && !theTouchInputManager.isTouched(0)) {
                RUNNER(sc->currentRunner)->jumpTimes.push_back(RUNNER(sc->currentRunner)->elapsed);
                RUNNER(sc->currentRunner)->jumpDurations.push_back(RunnerSystem::MaxJumpDuration);
                ANIMATION(sc->currentRunner)->playbackSpeed = 1.1;
                TEXT_RENDERING(entities->text)->text = "Your hero will run until the edge of the screen";
                return true;
            }
        }
        return false;
    }
};
