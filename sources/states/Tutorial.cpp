/*
   This file is part of RecursiveRunner.

   @author Soupe au Caillou - Pierre-Eric Pelloux-Prayer
   @author Soupe au Caillou - Gautier Pelloux-Prayer

   RecursiveRunner is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, version 3.

   RecursiveRunner is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with RecursiveRunner.  If not, see <http://www.gnu.org/licenses/>.
*/
#include "base/StateMachine.h"

#include "base/EntityManager.h"
#include "base/PlacementHelper.h"
#include "base/TouchInputManager.h"
#include "systems/TransformationSystem.h"
#include "systems/RenderingSystem.h"
#include "systems/AnimationSystem.h"
#include "systems/ADSRSystem.h"
#include "systems/ButtonSystem.h"
#include "systems/TextRenderingSystem.h"
#include "systems/SessionSystem.h"
#include "systems/PhysicsSystem.h"
#include "systems/RunnerSystem.h"
#include "systems/MusicSystem.h"
#include "api/LocalizeAPI.h"

#include "../RecursiveRunnerGame.h"
#include <glm/gtx/compatibility.hpp>

namespace Tutorial {
    enum Enum {
        Title,
        IntroduceHero,
        SmallJump,
        ScorePoints,
        BigJump,
        RunTilTheEdge,
        NewHero,
        MeetYourself,
        AvoidYourself,
        BestScore,
        TheEnd,
        Count
    };
}

struct TutorialEntities {
    Entity text;
    Entity anim;
};

struct TutorialStep {
    virtual ~TutorialStep() {}
    virtual void enter(LocalizeAPI* loc, SessionComponent* sc, TutorialEntities* entities) = 0;
    virtual bool mustUpdateGame(SessionComponent* sc, TutorialEntities* entities) = 0;
    virtual bool canExit(SessionComponent* sc, TutorialEntities* entities) = 0;
    virtual void exit(SessionComponent* sc, TutorialEntities* entities) = 0;

    void pointArrowTo(Entity arrow, const glm::vec2& target) const {
        glm::vec2 v = glm::normalize(target - TRANSFORM(arrow)->position);
        TRANSFORM(arrow)->rotation = glm::atan2(v.y, v.x);
    }
};

#include "tutorial/Title"
#include "tutorial/IntroduceHero"
#include "tutorial/SmallJump"
#include "tutorial/ScorePoints"
#include "tutorial/BigJump"
#include "tutorial/RunTilTheEdge"
#include "tutorial/NewHero"
#include "tutorial/MeetYourself"
#include "tutorial/AvoidYourself"
#include "tutorial/BestScore"
#include "tutorial/TheEnd"

class TutorialScene : public StateHandler<Scene::Enum> {
    RecursiveRunnerGame* game;
    // GameState* gameStateMgr;
    float waitBeforeEnterExit;
    TutorialEntities entities;
    Entity titleGroup, title, hideText;
    Tutorial::Enum currentStep;
    bool waitingClick;
    std::map<Tutorial::Enum, TutorialStep*> step2mgr;
    float flecheAccum;

public:

    TutorialScene(RecursiveRunnerGame* game) : StateHandler<Scene::Enum>() {
       this->game = game;
       // gameStateMgr = new GameState(game);
    }

    ~TutorialScene() {
        // delete gameStateMgr;
    }

    void setup() {
        // setup game
        // gameStateMgr->setup();
        // setup tutorial
        titleGroup  = theEntityManager.CreateEntity("tuto_title_group");
        ADD_COMPONENT(titleGroup, Transformation);
        TRANSFORM(titleGroup)->z = 0.7;
        TRANSFORM(titleGroup)->rotation = 0.036;
        TRANSFORM(titleGroup)->parent = game->cameraEntity;
        ADD_COMPONENT(titleGroup, ADSR);
        ADSR(titleGroup)->idleValue = (PlacementHelper::ScreenHeight + PlacementHelper::GimpSizeToScreen(theRenderingSystem.getTextureSize("titre")).y) * 0.6;
        ADSR(titleGroup)->sustainValue = (PlacementHelper::ScreenHeight - PlacementHelper::GimpSizeToScreen(theRenderingSystem.getTextureSize("titre")).y) * 0.5
            + PlacementHelper::GimpHeightToScreen(35);
        ADSR(titleGroup)->attackValue = ADSR(titleGroup)->sustainValue - PlacementHelper::GimpHeightToScreen(5);
        ADSR(titleGroup)->attackTiming = 1;
        ADSR(titleGroup)->decayTiming = 0.1;
        ADSR(titleGroup)->releaseTiming = 0.3;
        TRANSFORM(titleGroup)->position = glm::vec2(0, ADSR(titleGroup)->idleValue);

        title = theEntityManager.CreateEntity("tuto_title");
        ADD_COMPONENT(title, Transformation);
        TRANSFORM(title)->size = PlacementHelper::GimpSizeToScreen(theRenderingSystem.getTextureSize("taptostart"));
        TRANSFORM(title)->parent = titleGroup;
        TRANSFORM(title)->position = glm::vec2(0.0f);
        TRANSFORM(title)->z = 0.15;
        ADD_COMPONENT(title, Rendering);
        RENDERING(title)->texture = theRenderingSystem.loadTextureFile("taptostart");

        hideText = theEntityManager.CreateEntity("tuto_hide_text");
        ADD_COMPONENT(hideText, Transformation);
        TRANSFORM(hideText)->size = PlacementHelper::GimpSizeToScreen(glm::vec2(776, 102));
        TRANSFORM(hideText)->parent = title;
        TRANSFORM(hideText)->position = (glm::vec2(-0.5, 0.5) + glm::vec2(41, -72) / theRenderingSystem.getTextureSize("titre")) *
            TRANSFORM(title)->size + TRANSFORM(hideText)->size * glm::vec2(0.5, -0.5);
        TRANSFORM(hideText)->z = 0.01;
        ADD_COMPONENT(hideText, Rendering);
        RENDERING(hideText)->color = Color(130.0/255, 116.0/255, 117.0/255);

        entities.text = theEntityManager.CreateEntity("tuto_text");
        ADD_COMPONENT(entities.text, Transformation);
        TRANSFORM(entities.text)->size = TRANSFORM(hideText)->size;
        TRANSFORM(entities.text)->parent = hideText;
        TRANSFORM(entities.text)->z = 0.02;
        TRANSFORM(entities.text)->rotation = 0.004;
        ADD_COMPONENT(entities.text, TextRendering);
        TEXT_RENDERING(entities.text)->charHeight = PlacementHelper::GimpHeightToScreen(50);
        TEXT_RENDERING(entities.text)->show = false;
        TEXT_RENDERING(entities.text)->color = Color(40.0 / 255, 32.0/255, 30.0/255, 0.8);
        TEXT_RENDERING(entities.text)->positioning = TextRenderingComponent::CENTER;
        TEXT_RENDERING(entities.text)->flags |= TextRenderingComponent::AdjustHeightToFillWidthBit;

        entities.anim = theEntityManager.CreateEntity("tuto_fleche");
        ADD_COMPONENT(entities.anim, Transformation);
        TRANSFORM(entities.anim)->size = PlacementHelper::GimpSizeToScreen(theRenderingSystem.getTextureSize("fleche"));
        TRANSFORM(entities.anim)->z = 0.9;
        ADD_COMPONENT(entities.anim, Rendering);
        RENDERING(entities.anim)->texture = theRenderingSystem.loadTextureFile("fleche");
        RENDERING(entities.anim)->show = false;
        LOGW("TODO : use Animation for tutorial arrow blinking")
    }


    ///----------------------------------------------------------------------------//
    ///--------------------- ENTER SECTION ----------------------------------------//
    ///----------------------------------------------------------------------------//
    void onPreEnter(Scene::Enum) {
        #define INSTANCIATE_STEP(step) \
            step2mgr[Tutorial:: step] = new step##TutorialStep
        INSTANCIATE_STEP(Title);
        INSTANCIATE_STEP(IntroduceHero);
        INSTANCIATE_STEP(SmallJump);
        INSTANCIATE_STEP(ScorePoints);
        INSTANCIATE_STEP(BigJump);
        INSTANCIATE_STEP(RunTilTheEdge);
        INSTANCIATE_STEP(NewHero);
        INSTANCIATE_STEP(MeetYourself);
        INSTANCIATE_STEP(AvoidYourself);
        INSTANCIATE_STEP(BestScore);
        INSTANCIATE_STEP(TheEnd);
        assert(step2mgr.size() == (int)Tutorial::Count);

        flecheAccum = -1;
        bool isMuted = theMusicSystem.isMuted();
        theMusicSystem.toggleMute(true);
        // gameStateMgr->willEnter(Scene::Tutorial);
        theMusicSystem.toggleMute(isMuted);

        waitBeforeEnterExit = TimeUtil::GetTime();
        RENDERING(game->scorePanel)->show = TEXT_RENDERING(game->scoreText)->show = false;

        // hack lights/links
        SessionComponent* session = SESSION(theSessionSystem.RetrieveAllEntityWithComponent().front());
        std::for_each(session->coins.begin(), session->coins.end(), deleteEntityFunctor);
        session->coins.clear();
        std::for_each(session->links.begin(), session->links.end(), deleteEntityFunctor);
        session->links.clear();
        std::for_each(session->sparkling.begin(), session->sparkling.end(), deleteEntityFunctor);
        session->sparkling.clear();

        PlacementHelper::ScreenWidth = 60;
        PlacementHelper::GimpWidth = 3840;
        glm::vec2 c[] = {
            PlacementHelper::GimpPositionToScreen(glm::vec2(184, 568)),
            PlacementHelper::GimpPositionToScreen(glm::vec2(316, 448)),
            PlacementHelper::GimpPositionToScreen(glm::vec2(432, 612)),
            PlacementHelper::GimpPositionToScreen(glm::vec2(844, 600)),
            PlacementHelper::GimpPositionToScreen(glm::vec2(910, 450)),
            PlacementHelper::GimpPositionToScreen(glm::vec2(1046, 630)),
            PlacementHelper::GimpPositionToScreen(glm::vec2(1244, 564)),
            PlacementHelper::GimpPositionToScreen(glm::vec2(1340, 508)),
            PlacementHelper::GimpPositionToScreen(glm::vec2(1568, 460)),
            PlacementHelper::GimpPositionToScreen(glm::vec2(1820, 450)),
            PlacementHelper::GimpPositionToScreen(glm::vec2(1872, 580)),
            PlacementHelper::GimpPositionToScreen(glm::vec2(2096, 464)),
            PlacementHelper::GimpPositionToScreen(glm::vec2(2380, 584)),
            PlacementHelper::GimpPositionToScreen(glm::vec2(2540, 472)),
            PlacementHelper::GimpPositionToScreen(glm::vec2(2692, 572)),
            PlacementHelper::GimpPositionToScreen(glm::vec2(2916, 500)),
            PlacementHelper::GimpPositionToScreen(glm::vec2(3044, 568)),
            PlacementHelper::GimpPositionToScreen(glm::vec2(3560, 492)),
            PlacementHelper::GimpPositionToScreen(glm::vec2(3684, 566)),
            PlacementHelper::GimpPositionToScreen(glm::vec2(3776, 470))
        };
        std::vector<glm::vec2> coords;
        coords.resize(20);
        std::copy(c, &c[20], coords.begin());
        RecursiveRunnerGame::createCoins(coords, session, true);

        PlacementHelper::ScreenWidth = 20;
        PlacementHelper::GimpWidth = 1280;
        TEXT_RENDERING(entities.text)->text = game->gameThreadContext->localizeAPI->text("how_to_play");
        TEXT_RENDERING(entities.text)->show = false;

        BUTTON(game->muteBtn)->enabled = false;
    }

    bool updatePreEnter(Scene::Enum from, float) {
       // bool gameCanEnter = gameStateMgr->transitionCanEnter(from);

        if (TimeUtil::GetTime() - waitBeforeEnterExit < ADSR(titleGroup)->attackTiming) {
            return false;
        }
        // TRANSFORM(titleGroup)->position.X = theRenderingSystem.cameras[1].worldPosition.X;
        ADSR(titleGroup)->active = true;
        RENDERING(title)->show = true;
        //RENDERING(hideText)->show = true;

        ADSRComponent* adsr = ADSR(titleGroup);
        TRANSFORM(titleGroup)->position.y = adsr->value;
        RENDERING(game->muteBtn)->color.a = 1. - (adsr->value - adsr->idleValue) / (adsr->sustainValue - adsr->idleValue);

        // return (adsr->value == adsr->sustainValue) && gameCanEnter;
    }

    void onEnter(Scene::Enum) {
        SessionComponent* session = SESSION(theSessionSystem.RetrieveAllEntityWithComponent().front());
        session->userInputEnabled = false;
        TEXT_RENDERING(entities.text)->show = true;

        // gameStateMgr->enter(Scene::Tutorial);
        waitingClick = true;
        currentStep = Tutorial::Title;
        step2mgr[currentStep]->enter(game->gameThreadContext->localizeAPI, session, &entities);
        RENDERING(game->muteBtn)->show = RENDERING(game->scorePanel)->show = TEXT_RENDERING(game->scoreText)->show = false;
    }


    ///----------------------------------------------------------------------------//
    ///--------------------- UPDATE SECTION ---------------------------------------//
    ///----------------------------------------------------------------------------//

    Scene::Enum update(float dt) {
        SessionComponent* session = SESSION(theSessionSystem.RetrieveAllEntityWithComponent().front());
        if (waitingClick) {
            if (flecheAccum >= 0) {
                flecheAccum += dt * 2;
                RENDERING(entities.anim)->show = !(((int)flecheAccum) % 2);
            }

            if (!game->ignoreClick && theTouchInputManager.wasTouched(0) && !theTouchInputManager.isTouched(0)) {
                waitingClick = false;
                step2mgr[currentStep]->exit(session, &entities);
                if (currentStep == Tutorial::TheEnd) {
                    return Scene::Menu;
                } else {
                    currentStep = (Tutorial::Enum) (currentStep + 1);
                    // step2mgr[currentStep]->enter(game->localizeAPI, session, &entities);
                    TEXT_RENDERING(entities.text)->show = true;
                    TEXT_RENDERING(entities.text)->text = ". . .";
                    RENDERING(entities.anim)->show = false;
                    flecheAccum = 0;
                }
            }
        } else {
            flecheAccum += dt * 3;
            TEXT_RENDERING(entities.text)->show = !(((int)flecheAccum) % 2);

            if (step2mgr[currentStep]->mustUpdateGame(session, &entities)) {
                // gameStateMgr->update(dt);
            }
            if (step2mgr[currentStep]->canExit(session, &entities)) {
                // this is not a bug, enter method need to be renamed
                step2mgr[currentStep]->enter(game->gameThreadContext->localizeAPI, session, &entities);
                TEXT_RENDERING(entities.text)->show = true;
                flecheAccum = RENDERING(entities.anim)->show ? 0 : -1;
                waitingClick = true;
            }
        }
        return Scene::Tutorial;
    }


    ///----------------------------------------------------------------------------//
    ///--------------------- EXIT SECTION -----------------------------------------//
    ///----------------------------------------------------------------------------//
    void onPreExit(Scene::Enum to) {
        SessionComponent* session = SESSION(theSessionSystem.RetrieveAllEntityWithComponent().front());
        session->userInputEnabled = false;
        RENDERING(hideText)->show = false;
        TEXT_RENDERING(entities.text)->show = false;
        // gameStateMgr->willExit(to);


        RENDERING(game->muteBtn)->show = RENDERING(game->scorePanel)->show = TEXT_RENDERING(game->scoreText)->show = true;
    }

    bool updatePreExit(Scene::Enum to, float) {

        ADSRComponent* adsr = ADSR(titleGroup);
        adsr->active = false;

        TRANSFORM(titleGroup)->position.y = adsr->value;

        RENDERING(game->muteBtn)->color.a = (adsr->sustainValue - adsr->value) / (adsr->sustainValue - adsr->idleValue);

        // return gameStateMgr->transitionCanExit(to);
    }

    void onExit(Scene::Enum to) {
        RENDERING(title)->show = false;

        TEXT_RENDERING(entities.text)->show = false;
        RENDERING(entities.anim)->show = false;
        // gameStateMgr->exit(to);
        for(std::map<Tutorial::Enum, TutorialStep*>::iterator it=step2mgr.begin();
            it!=step2mgr.end();
            ++it) {
            delete it->second;
        }
        step2mgr.clear();
        BUTTON(game->muteBtn)->enabled = true;
    }
};

namespace Scene {
    StateHandler<Scene::Enum>* CreateTutorialSceneHandler(RecursiveRunnerGame* game) {
        return new TutorialScene(game);
    }
}

