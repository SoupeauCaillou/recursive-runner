#pragma once

struct ScorePointsTutorialStep : public TutorialStep {
    void enter(SessionComponent* sc, TutorialEntities* entities) {
        TEXT_RENDERING(entities->text)->hide = true;
    }
    bool mustUpdateGame(SessionComponent* sc, TutorialEntities* entities) {
        if (PHYSICS(sc->currentRunner)->linearVelocity.Y < 0) { // check coins instead
            ANIMATION(sc->currentRunner)->playbackSpeed = 0;
            TEXT_RENDERING(entities->text)->text = "Turn on lights to score points";
            TEXT_RENDERING(entities->text)->hide = false;
            PHYSICS(sc->currentRunner)->mass = 0;
            return false;
        } else {
            return true;
        }
    }
    bool canExit(SessionComponent* sc, TutorialEntities* entities) {
        if (theTouchInputManager.wasTouched(0) && !theTouchInputManager.isTouched(0)) {
            ANIMATION(sc->currentRunner)->playbackSpeed = 1.1;
            PHYSICS(sc->currentRunner)->mass = 1;
            return true;
        } else {
            return false;
        }
    }
};
