#pragma once

struct AvoidYourselfTutorialStep : public TutorialStep {
    void enter(SessionComponent* sc, TutorialEntities* entities) {
        TEXT_RENDERING(entities->text)->hide = true;
        TEXT_RENDERING(entities->text)->text = "Avoid your old self they still score points";
        TEXT_RENDERING(entities->text)->positioning = TextRenderingComponent::RIGHT;
    }
    bool mustUpdateGame(SessionComponent* sc, TutorialEntities* entities) {
        if (Vector2::Distance(TRANSFORM(sc->runners[0])->position, TRANSFORM(sc->runners[1])->position) > 4) {
            return true;
        } else {
            TEXT_RENDERING(entities->text)->hide = false;
            ANIMATION(sc->runners[0])->playbackSpeed = 0;
            ANIMATION(sc->runners[1])->playbackSpeed = 0;
            TRANSFORM(entities->text)->parent = sc->runners[1];
            return false;
        }
    }
    bool canExit(SessionComponent* sc, TutorialEntities* entities) {
        if (ANIMATION(sc->currentRunner)->playbackSpeed == 0) {
            if (theTouchInputManager.wasTouched(0) && !theTouchInputManager.isTouched(0)) {
                RUNNER(sc->currentRunner)->jumpTimes.push_back(RUNNER(sc->currentRunner)->elapsed);
                RUNNER(sc->currentRunner)->jumpDurations.push_back(RunnerSystem::MaxJumpDuration);
                ANIMATION(sc->runners[0])->playbackSpeed = ANIMATION(sc->runners[1])->playbackSpeed = 1.1;
                return true;
            }
        }
        return false;
    }
};
