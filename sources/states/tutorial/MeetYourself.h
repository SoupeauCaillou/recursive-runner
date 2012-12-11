#pragma once

struct MeetYourselfTutorialStep : public TutorialStep {
    void enter(SessionComponent* sc, TutorialEntities* entities) {
        TEXT_RENDERING(entities->text)->hide = true;
        TEXT_RENDERING(entities->text)->text = "This is old You repeating the same run";
        TEXT_RENDERING(entities->text)->positioning = TextRenderingComponent::LEFT;
    }
    bool mustUpdateGame(SessionComponent* sc, TutorialEntities* entities) {
        if (Vector2::Distance(TRANSFORM(sc->runners[0])->position, TRANSFORM(sc->runners[1])->position) > theRenderingSystem.cameras[1].worldSize.X * 0.8) {
            return true;
        } else {
            TEXT_RENDERING(entities->text)->hide = false;
            ANIMATION(sc->runners[0])->playbackSpeed = 0;
            ANIMATION(sc->runners[1])->playbackSpeed = 0;
            TRANSFORM(entities->text)->parent = sc->runners[0];
            return false;
        }
    }
    bool canExit(SessionComponent* sc, TutorialEntities* entities) {
        if (ANIMATION(sc->currentRunner)->playbackSpeed == 0) {
            if (theTouchInputManager.wasTouched(0) && !theTouchInputManager.isTouched(0)) {
                ANIMATION(sc->runners[0])->playbackSpeed = ANIMATION(sc->runners[1])->playbackSpeed = 1.1;
                return true;
            }
        }
        return false;
    }
};
