#pragma once

struct IntroduceHeroTutorialStep : public TutorialStep {
    void enter(SessionComponent* sc, TutorialEntities* entities) {
        TEXT_RENDERING(entities->text)->hide = false;
        TEXT_RENDERING(entities->text)->text = "This is You";
    }
    bool mustUpdateGame(SessionComponent* , TutorialEntities*) {
        return true;
    }
    bool canExit(SessionComponent* sc, TutorialEntities*) {
        if (TRANSFORM(sc->currentRunner)->position.X >= -15) {
            return true;
        } else {
            return false;
        }
    }
};
