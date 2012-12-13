#pragma once

struct SmallJumpTutorialStep : public TutorialStep {
    void enter(SessionComponent* sc, TutorialEntities* entities) {
        ANIMATION(sc->currentRunner)->playbackSpeed = 0;
        TEXT_RENDERING(entities->text)->hide = false;
        TEXT_RENDERING(entities->text)->text = "Tap the screen to jump";
    }
    bool mustUpdateGame(SessionComponent*, TutorialEntities*) {
        return false;
    }
    bool canExit(SessionComponent* sc, TutorialEntities* ) {
        if (theTouchInputManager.wasTouched(0) && !theTouchInputManager.isTouched(0)) {
            RUNNER(sc->currentRunner)->jumpTimes.push_back(RUNNER(sc->currentRunner)->elapsed);
            RUNNER(sc->currentRunner)->jumpDurations.push_back(0.06);//RunnerSystem::MinJumpDuration);
            ANIMATION(sc->currentRunner)->playbackSpeed = 1.1;
            return true;
        } else {
            return false;
        }
    }
};
