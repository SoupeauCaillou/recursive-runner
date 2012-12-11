#pragma once

struct IntroduceHeroTutorialStep : public TutorialStep {
    void enter(SessionComponent* sc, TutorialEntities* entities) {
        TRANSFORM(entities->text)->parent = sc->currentRunner;
        TRANSFORM(entities->text)->z = 0.1;
        TRANSFORM(entities->text)->position = Vector2(0, TRANSFORM(sc->currentRunner)->size.Y * 2);
        TEXT_RENDERING(entities->text)->hide = false;
        TEXT_RENDERING(entities->text)->text = "This is your hero";
        TEXT_RENDERING(entities->text)->positioning = TextRenderingComponent::LEFT;
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
