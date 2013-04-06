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
#include "StateManager.h"

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

struct TutorialState::TutorialStateDatas {
    GameState* gameStateMgr;
    float waitBeforeEnterExit;
    TutorialEntities entities;
    Entity titleGroup, title, hideText;
    Tutorial::Enum currentStep;
    bool waitingClick;
    std::map<Tutorial::Enum, TutorialStep*> step2mgr;
    float flecheAccum;
};

TutorialState::TutorialState(RecursiveRunnerGame* game) : StateManager(State::Tutorial, game) {
   datas = new TutorialStateDatas;
   datas->gameStateMgr = new GameState(game);
}

TutorialState::~TutorialState() {
    delete datas->gameStateMgr;

    delete datas;
}

void TutorialState::setup() {
    // setup game
    datas->gameStateMgr->setup();
    // setup tutorial
    Entity titleGroup = datas->titleGroup  = theEntityManager.CreateEntity("tuto_title_group");
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

    Entity title = datas->title = theEntityManager.CreateEntity("tuto_title");
    ADD_COMPONENT(title, Transformation);
    TRANSFORM(title)->size = PlacementHelper::GimpSizeToScreen(theRenderingSystem.getTextureSize("taptostart"));
    TRANSFORM(title)->parent = titleGroup;
    TRANSFORM(title)->position = glm::vec2(0.0f);
    TRANSFORM(title)->z = 0.15;
    ADD_COMPONENT(title, Rendering);
    RENDERING(title)->texture = theRenderingSystem.loadTextureFile("taptostart");

    Entity hideText = datas->hideText = theEntityManager.CreateEntity("tuto_hide_text");
    ADD_COMPONENT(hideText, Transformation);
    TRANSFORM(hideText)->size = PlacementHelper::GimpSizeToScreen(glm::vec2(776, 102));
    TRANSFORM(hideText)->parent = title;
    TRANSFORM(hideText)->position = (glm::vec2(-0.5, 0.5) + glm::vec2(41, -72) / theRenderingSystem.getTextureSize("titre")) *
        TRANSFORM(title)->size + TRANSFORM(hideText)->size * glm::vec2(0.5, -0.5);
    TRANSFORM(hideText)->z = 0.01;
    ADD_COMPONENT(hideText, Rendering);
    RENDERING(hideText)->color = Color(130.0/255, 116.0/255, 117.0/255);

    Entity text = datas->entities.text = theEntityManager.CreateEntity("tuto_text");
    ADD_COMPONENT(text, Transformation);
    TRANSFORM(text)->size = TRANSFORM(hideText)->size;
    TRANSFORM(text)->parent = hideText;
    TRANSFORM(text)->z = 0.02;
    TRANSFORM(text)->rotation = 0.004;
    ADD_COMPONENT(text, TextRendering);
    TEXT_RENDERING(text)->charHeight = PlacementHelper::GimpHeightToScreen(50);
    TEXT_RENDERING(text)->show = false;
    TEXT_RENDERING(text)->color = Color(40.0 / 255, 32.0/255, 30.0/255, 0.8);
    TEXT_RENDERING(text)->positioning = TextRenderingComponent::CENTER;
    TEXT_RENDERING(text)->flags |= TextRenderingComponent::AdjustHeightToFillWidthBit;

    Entity anim = datas->entities.anim = theEntityManager.CreateEntity("tuto_fleche");
    ADD_COMPONENT(anim, Transformation);
    TRANSFORM(anim)->size = PlacementHelper::GimpSizeToScreen(theRenderingSystem.getTextureSize("fleche"));
    TRANSFORM(anim)->z = 0.9;
    ADD_COMPONENT(anim, Rendering);
    RENDERING(anim)->texture = theRenderingSystem.loadTextureFile("fleche");
    RENDERING(anim)->show = false;
    // ADD_COMPONENT(anim, Animation);
}


///----------------------------------------------------------------------------//
///--------------------- ENTER SECTION ----------------------------------------//
///----------------------------------------------------------------------------//
void TutorialState::willEnter(State::Enum) {
    #define INSTANCIATE_STEP(step) \
        datas->step2mgr[Tutorial:: step] = new step##TutorialStep
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
    assert(datas->step2mgr.size() == (int)Tutorial::Count);

    datas->flecheAccum = -1;
    bool isMuted = theMusicSystem.isMuted();
    theMusicSystem.toggleMute(true);
    datas->gameStateMgr->willEnter(State::Tutorial);
    theMusicSystem.toggleMute(isMuted);

    datas->waitBeforeEnterExit = TimeUtil::GetTime();
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
    TEXT_RENDERING(datas->entities.text)->text = game->gameThreadContext->localizeAPI->text("how_to_play", "How to play? (tap to continue)");
    TEXT_RENDERING(datas->entities.text)->show = false;

    BUTTON(game->muteBtn)->enabled = false;
}

bool TutorialState::transitionCanEnter(State::Enum from) {
   bool gameCanEnter = datas->gameStateMgr->transitionCanEnter(from);

    if (TimeUtil::GetTime() - datas->waitBeforeEnterExit < ADSR(datas->titleGroup)->attackTiming) {
        return false;
    }
    // TRANSFORM(datas->titleGroup)->position.X = theRenderingSystem.cameras[1].worldPosition.X;
    ADSR(datas->titleGroup)->active = true;
    RENDERING(datas->title)->show = true;
    //RENDERING(datas->hideText)->show = true;

    ADSRComponent* adsr = ADSR(datas->titleGroup);
    TRANSFORM(datas->titleGroup)->position.y = adsr->value;
    RENDERING(game->muteBtn)->color.a = 1. - (adsr->value - adsr->idleValue) / (adsr->sustainValue - adsr->idleValue);

    return (adsr->value == adsr->sustainValue) && gameCanEnter;
}

void TutorialState::enter(State::Enum) {
    SessionComponent* session = SESSION(theSessionSystem.RetrieveAllEntityWithComponent().front());
    session->userInputEnabled = false;
    TEXT_RENDERING(datas->entities.text)->show = true;

    datas->gameStateMgr->enter(State::Tutorial);
    datas->waitingClick = true;
    datas->currentStep = Tutorial::Title;
    datas->step2mgr[datas->currentStep]->enter(game->gameThreadContext->localizeAPI, session, &datas->entities);
    RENDERING(game->muteBtn)->show = RENDERING(game->scorePanel)->show = TEXT_RENDERING(game->scoreText)->show = false;
}


///----------------------------------------------------------------------------//
///--------------------- UPDATE SECTION ---------------------------------------//
///----------------------------------------------------------------------------//
void TutorialState::backgroundUpdate(float) {
}

State::Enum TutorialState::update(float dt) {
    SessionComponent* session = SESSION(theSessionSystem.RetrieveAllEntityWithComponent().front());
    if (datas->waitingClick) {
        if (datas->flecheAccum >= 0) {
            datas->flecheAccum += dt * 2;
            RENDERING(datas->entities.anim)->show = !(((int)datas->flecheAccum) % 2);
        }

        if (!game->ignoreClick && theTouchInputManager.wasTouched(0) && !theTouchInputManager.isTouched(0)) {
            datas->waitingClick = false;
            datas->step2mgr[datas->currentStep]->exit(session, &datas->entities);
            if (datas->currentStep == Tutorial::TheEnd) {
                return State::Menu;
            } else {
                datas->currentStep = (Tutorial::Enum) (datas->currentStep + 1);
                // datas->step2mgr[datas->currentStep]->enter(game->localizeAPI, session, &datas->entities);
                TEXT_RENDERING(datas->entities.text)->show = true;
                TEXT_RENDERING(datas->entities.text)->text = ". . .";
                RENDERING(datas->entities.anim)->show = false;
                datas->flecheAccum = 0;
            }
        }
    } else {
        datas->flecheAccum += dt * 3;
        TEXT_RENDERING(datas->entities.text)->show = !(((int)datas->flecheAccum) % 2);

        if (datas->step2mgr[datas->currentStep]->mustUpdateGame(session, &datas->entities)) {
            datas->gameStateMgr->update(dt);
        }
        if (datas->step2mgr[datas->currentStep]->canExit(session, &datas->entities)) {
            // this is not a bug, enter method need to be renamed
            datas->step2mgr[datas->currentStep]->enter(game->gameThreadContext->localizeAPI, session, &datas->entities);
            TEXT_RENDERING(datas->entities.text)->show = true;
            datas->flecheAccum = RENDERING(datas->entities.anim)->show ? 0 : -1;
            datas->waitingClick = true;
        }
    }
    return State::Tutorial;
}


///----------------------------------------------------------------------------//
///--------------------- EXIT SECTION -----------------------------------------//
///----------------------------------------------------------------------------//
void TutorialState::willExit(State::Enum to) {
    SessionComponent* session = SESSION(theSessionSystem.RetrieveAllEntityWithComponent().front());
    session->userInputEnabled = false;
    RENDERING(datas->hideText)->show = false;
    TEXT_RENDERING(datas->entities.text)->show = false;
    datas->gameStateMgr->willExit(to);


    RENDERING(game->muteBtn)->show = RENDERING(game->scorePanel)->show = TEXT_RENDERING(game->scoreText)->show = true;
}

bool TutorialState::transitionCanExit(State::Enum to) {

    ADSRComponent* adsr = ADSR(datas->titleGroup);
    adsr->active = false;

    TRANSFORM(datas->titleGroup)->position.y = adsr->value;

    RENDERING(game->muteBtn)->color.a = (adsr->sustainValue - adsr->value) / (adsr->sustainValue - adsr->idleValue);

    return datas->gameStateMgr->transitionCanExit(to);
}

void TutorialState::exit(State::Enum to) {
    RENDERING(datas->title)->show = false;

    TEXT_RENDERING(datas->entities.text)->show = false;
    RENDERING(datas->entities.anim)->show = false;
    datas->gameStateMgr->exit(to);
    for(std::map<Tutorial::Enum, TutorialStep*>::iterator it=datas->step2mgr.begin();
        it!=datas->step2mgr.end();
        ++it) {
        delete it->second;
    }
    datas->step2mgr.clear();
    BUTTON(game->muteBtn)->enabled = true;
}

