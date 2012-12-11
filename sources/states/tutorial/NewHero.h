#pragma once

struct NewHeroTutorialStep : public TutorialStep {
    void enter(SessionComponent* sc, TutorialEntities* entities) {

    }
    bool mustUpdateGame(SessionComponent* sc, TutorialEntities* entities) {
        if (sc->runners.size() == 1) {
            float a = MathUtil::Max(0.0f, MathUtil::Min(1.0f, (TRANSFORM(sc->currentRunner)->position.X - 0) / (30 - 0)));
            TEXT_RENDERING(entities->text)->positioning = a;//TextRenderingComponent::RIGHT;
            return true;
        } else if (TRANSFORM(sc->currentRunner)->position.X > 29) {
            RUNNER(sc->runners[0])->startTime = 0;
            return true;
        } else {
            TEXT_RENDERING(entities->text)->text = "This is the new You";
            TEXT_RENDERING(entities->text)->positioning = TextRenderingComponent::RIGHT;
            TEXT_RENDERING(entities->text)->hide = false;
            ANIMATION(sc->currentRunner)->playbackSpeed = 0;
            TRANSFORM(entities->text)->parent = sc->currentRunner;
            return false;
        }
    }
    bool canExit(SessionComponent* sc, TutorialEntities* entities) {
        if (ANIMATION(sc->currentRunner)->playbackSpeed == 0) {
            if (theTouchInputManager.wasTouched(0) && !theTouchInputManager.isTouched(0)) {
                ANIMATION(sc->currentRunner)->playbackSpeed = 1.1;
                return true;
            }
        }
        return false;
    }
};
