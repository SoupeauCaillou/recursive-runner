#pragma once

struct NewHeroTutorialStep : public TutorialStep {
    void enter(SessionComponent* , TutorialEntities* ) {

    }
    bool mustUpdateGame(SessionComponent* sc, TutorialEntities* entities) {
        if (sc->runners.size() == 1) {
            return true;
        } else if (TRANSFORM(sc->currentRunner)->position.X > 29) {
            RUNNER(sc->runners[0])->startTime = 0;
            return true;
        } else {
            TEXT_RENDERING(entities->text)->text = "This is the your new hero";
            ANIMATION(sc->currentRunner)->playbackSpeed = 0;
            return false;
        }
    }
    bool canExit(SessionComponent* sc, TutorialEntities* ) {
        if (ANIMATION(sc->currentRunner)->playbackSpeed == 0) {
            if (theTouchInputManager.wasTouched(0) && !theTouchInputManager.isTouched(0)) {
                ANIMATION(sc->currentRunner)->playbackSpeed = 1.1;
                return true;
            }
        }
        return false;
    }
};
