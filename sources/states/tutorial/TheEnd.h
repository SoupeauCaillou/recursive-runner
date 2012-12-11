#pragma once

struct TheEndTutorialStep : public TutorialStep {
    void enter(SessionComponent* sc, TutorialEntities* entities) {
        TEXT_RENDERING(entities->text)->hide = false;
        TEXT_RENDERING(entities->text)->text = "You ll have 10 runners";
        TRANSFORM(entities->text)->parent = sc->currentRunner;
        TEXT_RENDERING(entities->text)->positioning = TextRenderingComponent::CENTER;
    }
    bool mustUpdateGame(SessionComponent* sc, TutorialEntities* entities) {
        if (TRANSFORM(sc->currentRunner)->position.X < -15) {
            TEXT_RENDERING(entities->text)->text = "Turn on lights and score a lot";
            TEXT_RENDERING(entities->text)->positioning = TextRenderingComponent::LEFT;
        }
        if (TRANSFORM(sc->currentRunner)->position.X > -29) {
            return true;
        } else {
            TEXT_RENDERING(entities->text)->text = "End of tutorial good luck";
            TEXT_RENDERING(entities->text)->hide = false;
            ANIMATION(sc->currentRunner)->playbackSpeed = 0;
            return false;
        }
    }
    bool canExit(SessionComponent* sc, TutorialEntities* entities) {
        if (ANIMATION(sc->currentRunner)->playbackSpeed == 0) {
            if (theTouchInputManager.wasTouched(0) && !theTouchInputManager.isTouched(0)) {
                TEXT_RENDERING(entities->text)->hide = true;
                TRANSFORM(entities->text)->parent = 0;
                return true;
            }
        }
        return false;
    }
};
