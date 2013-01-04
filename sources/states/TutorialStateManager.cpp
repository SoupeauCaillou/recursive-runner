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

struct TutorialStateManager::TutorialStateManagerDatas {
    GameStateManager* gameStateMgr;
    float waitBeforeEnterExit;
    TutorialEntities entities;
    Entity titleGroup, title, hideText;
    Tutorial::Enum currentStep;
    bool waitingClick;
    std::map<Tutorial::Enum, TutorialStep*> step2mgr;
    float flecheAccum;
};

TutorialStateManager::TutorialStateManager(RecursiveRunnerGame* game) : StateManager(State::Tutorial, game) {
   datas = new TutorialStateManagerDatas;
   datas->gameStateMgr = new GameStateManager(game);
}

TutorialStateManager::~TutorialStateManager() {
    delete datas->gameStateMgr;

    delete datas;
}

void TutorialStateManager::setup() {
    // setup game
    datas->gameStateMgr->setup();
    // setup tutorial
    Entity titleGroup = datas->titleGroup  = theEntityManager.CreateEntity();
    ADD_COMPONENT(titleGroup, Transformation);
    TRANSFORM(titleGroup)->z = 0.7;
    TRANSFORM(titleGroup)->rotation = 0.036;
    TRANSFORM(titleGroup)->parent = game->cameraEntity;
    ADD_COMPONENT(titleGroup, ADSR);
    ADSR(titleGroup)->idleValue = (PlacementHelper::ScreenHeight + PlacementHelper::GimpSizeToScreen(theRenderingSystem.getTextureSize("titre")).Y) * 0.6;
    ADSR(titleGroup)->sustainValue = (PlacementHelper::ScreenHeight - PlacementHelper::GimpSizeToScreen(theRenderingSystem.getTextureSize("titre")).Y) * 0.5
        + PlacementHelper::GimpHeightToScreen(35);
    ADSR(titleGroup)->attackValue = ADSR(titleGroup)->sustainValue - PlacementHelper::GimpHeightToScreen(5);
    ADSR(titleGroup)->attackTiming = 1;
    ADSR(titleGroup)->decayTiming = 0.1;
    ADSR(titleGroup)->releaseTiming = 0.3;
    TRANSFORM(titleGroup)->position = Vector2(0, ADSR(titleGroup)->idleValue);

    Entity title = datas->title = theEntityManager.CreateEntity();
    ADD_COMPONENT(title, Transformation);
    TRANSFORM(title)->size = PlacementHelper::GimpSizeToScreen(theRenderingSystem.getTextureSize("taptostart"));
    TRANSFORM(title)->parent = titleGroup;
    TRANSFORM(title)->position = Vector2::Zero;
    TRANSFORM(title)->z = 0.15;
    ADD_COMPONENT(title, Rendering);
    RENDERING(title)->texture = theRenderingSystem.loadTextureFile("taptostart");
    RENDERING(title)->cameraBitMask = 0x3;

    Entity hideText = datas->hideText = theEntityManager.CreateEntity();
    ADD_COMPONENT(hideText, Transformation);
    TRANSFORM(hideText)->size = PlacementHelper::GimpSizeToScreen(Vector2(776, 102));
    TRANSFORM(hideText)->parent = title;
    TRANSFORM(hideText)->position = (Vector2(-0.5, 0.5) + Vector2(41, -72) / theRenderingSystem.getTextureSize("titre")) *
        TRANSFORM(title)->size + TRANSFORM(hideText)->size * Vector2(0.5, -0.5);
    TRANSFORM(hideText)->z = 0.01;
    ADD_COMPONENT(hideText, Rendering);
    RENDERING(hideText)->color = Color(130.0/255, 116.0/255, 117.0/255);
    RENDERING(hideText)->cameraBitMask = 0x3;

    Entity text = datas->entities.text = theEntityManager.CreateEntity();
    ADD_COMPONENT(text, Transformation);
    TRANSFORM(text)->size = TRANSFORM(hideText)->size;
    TRANSFORM(text)->parent = hideText;
    TRANSFORM(text)->z = 0.02;
    TRANSFORM(text)->rotation = 0.004;
    ADD_COMPONENT(text, TextRendering);
    TEXT_RENDERING(text)->charHeight = PlacementHelper::GimpHeightToScreen(50);
    TEXT_RENDERING(text)->hide = true;
    TEXT_RENDERING(text)->cameraBitMask = 0x3;
    TEXT_RENDERING(text)->color = Color(40.0 / 255, 32.0/255, 30.0/255, 0.8);
    TEXT_RENDERING(text)->positioning = TextRenderingComponent::CENTER;
    TEXT_RENDERING(text)->flags |= TextRenderingComponent::AdjustHeightToFillWidthBit;

    Entity anim = datas->entities.anim = theEntityManager.CreateEntity();
    ADD_COMPONENT(anim, Transformation);
    TRANSFORM(anim)->size = PlacementHelper::GimpSizeToScreen(theRenderingSystem.getTextureSize("fleche"));
    TRANSFORM(anim)->z = 0.9;
    ADD_COMPONENT(anim, Rendering);
    RENDERING(anim)->cameraBitMask = 0x3;
    RENDERING(anim)->texture = theRenderingSystem.loadTextureFile("fleche");
    RENDERING(anim)->hide = true;
    // ADD_COMPONENT(anim, Animation);
}


///----------------------------------------------------------------------------//
///--------------------- ENTER SECTION ----------------------------------------//
///----------------------------------------------------------------------------//
void TutorialStateManager::willEnter(State::Enum) {
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

    datas->waitBeforeEnterExit = TimeUtil::getTime();
    RENDERING(game->scorePanel)->hide = TEXT_RENDERING(game->scoreText)->hide = true;

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
    Vector2 c[] = {
        PlacementHelper::GimpPositionToScreen(Vector2(184, 568)),
        PlacementHelper::GimpPositionToScreen(Vector2(316, 448)),
        PlacementHelper::GimpPositionToScreen(Vector2(432, 612)),
        PlacementHelper::GimpPositionToScreen(Vector2(844, 600)),
        PlacementHelper::GimpPositionToScreen(Vector2(910, 450)),
        PlacementHelper::GimpPositionToScreen(Vector2(1046, 630)),
        PlacementHelper::GimpPositionToScreen(Vector2(1244, 564)),
        PlacementHelper::GimpPositionToScreen(Vector2(1340, 508)),
        PlacementHelper::GimpPositionToScreen(Vector2(1568, 460)),
        PlacementHelper::GimpPositionToScreen(Vector2(1820, 450)),
        PlacementHelper::GimpPositionToScreen(Vector2(1872, 580)),
        PlacementHelper::GimpPositionToScreen(Vector2(2096, 464)),
        PlacementHelper::GimpPositionToScreen(Vector2(2380, 584)),
        PlacementHelper::GimpPositionToScreen(Vector2(2540, 472)),
        PlacementHelper::GimpPositionToScreen(Vector2(2692, 572)),
        PlacementHelper::GimpPositionToScreen(Vector2(2916, 500)),
        PlacementHelper::GimpPositionToScreen(Vector2(3044, 568)),
        PlacementHelper::GimpPositionToScreen(Vector2(3560, 492)),
        PlacementHelper::GimpPositionToScreen(Vector2(3684, 566)),
        PlacementHelper::GimpPositionToScreen(Vector2(3776, 470))
    };
    std::vector<Vector2> coords;
    coords.resize(20);
    std::copy(c, &c[20], coords.begin());
    RecursiveRunnerGame::createCoins(coords, session, true);

    PlacementHelper::ScreenWidth = 20;
    PlacementHelper::GimpWidth = 1280;
    TEXT_RENDERING(datas->entities.text)->text = game->localizeAPI->text("how_to_play", "How to play? (tap to continue)");
    TEXT_RENDERING(datas->entities.text)->hide = true;

    BUTTON(game->muteBtn)->enabled = false;
}

bool TutorialStateManager::transitionCanEnter(State::Enum from) {
   bool gameCanEnter = datas->gameStateMgr->transitionCanEnter(from);

    if (TimeUtil::getTime() - datas->waitBeforeEnterExit < ADSR(datas->titleGroup)->attackTiming) {
        return false;
    }
    // TRANSFORM(datas->titleGroup)->position.X = theRenderingSystem.cameras[1].worldPosition.X;
    ADSR(datas->titleGroup)->active = true;
    RENDERING(datas->title)->hide = false;
    //RENDERING(datas->hideText)->hide = false;

    ADSRComponent* adsr = ADSR(datas->titleGroup);
    TRANSFORM(datas->titleGroup)->position.Y = adsr->value;
    RENDERING(game->muteBtn)->color.a = 1. - (adsr->value - adsr->idleValue) / (adsr->sustainValue - adsr->idleValue);

    return (adsr->value == adsr->sustainValue) && gameCanEnter;
}

void TutorialStateManager::enter(State::Enum) {
    SessionComponent* session = SESSION(theSessionSystem.RetrieveAllEntityWithComponent().front());
    session->userInputEnabled = false;
    TEXT_RENDERING(datas->entities.text)->hide = false;

    datas->gameStateMgr->enter(State::Tutorial);
    datas->waitingClick = true;
    datas->currentStep = Tutorial::Title;
    datas->step2mgr[datas->currentStep]->enter(game->localizeAPI, session, &datas->entities);
    RENDERING(game->muteBtn)->hide = RENDERING(game->scorePanel)->hide = TEXT_RENDERING(game->scoreText)->hide = true;
}


///----------------------------------------------------------------------------//
///--------------------- UPDATE SECTION ---------------------------------------//
///----------------------------------------------------------------------------//
void TutorialStateManager::backgroundUpdate(float) {
}

State::Enum TutorialStateManager::update(float dt) {
    SessionComponent* session = SESSION(theSessionSystem.RetrieveAllEntityWithComponent().front());
    if (datas->waitingClick) {
        if (datas->flecheAccum >= 0) {
            datas->flecheAccum += dt * 2;
            RENDERING(datas->entities.anim)->hide = (((int)datas->flecheAccum) % 2);
        }

        if (!game->ignoreClick && theTouchInputManager.wasTouched(0) && !theTouchInputManager.isTouched(0)) {
            datas->waitingClick = false;
            datas->step2mgr[datas->currentStep]->exit(session, &datas->entities);
            if (datas->currentStep == Tutorial::TheEnd) {
                return State::Menu;
            } else {
                datas->currentStep = (Tutorial::Enum) (datas->currentStep + 1);
                // datas->step2mgr[datas->currentStep]->enter(game->localizeAPI, session, &datas->entities);
                TEXT_RENDERING(datas->entities.text)->hide = false;
                TEXT_RENDERING(datas->entities.text)->text = ". . .";
                RENDERING(datas->entities.anim)->hide = true;
                datas->flecheAccum = 0;
            }
        }
    } else {
        datas->flecheAccum += dt * 3;
        TEXT_RENDERING(datas->entities.text)->hide = (((int)datas->flecheAccum) % 2);

        if (datas->step2mgr[datas->currentStep]->mustUpdateGame(session, &datas->entities)) {
            datas->gameStateMgr->update(dt);
        }
        if (datas->step2mgr[datas->currentStep]->canExit(session, &datas->entities)) {
            // this is not a bug, enter method need to be renamed
            datas->step2mgr[datas->currentStep]->enter(game->localizeAPI, session, &datas->entities);
            TEXT_RENDERING(datas->entities.text)->hide = false;
            datas->flecheAccum = RENDERING(datas->entities.anim)->hide ? -1 : 0;
            datas->waitingClick = true;
        }
    }
    return State::Tutorial;
}


///----------------------------------------------------------------------------//
///--------------------- EXIT SECTION -----------------------------------------//
///----------------------------------------------------------------------------//
void TutorialStateManager::willExit(State::Enum to) {
    SessionComponent* session = SESSION(theSessionSystem.RetrieveAllEntityWithComponent().front());
    session->userInputEnabled = false;
    RENDERING(datas->hideText)->hide = true;
    TEXT_RENDERING(datas->entities.text)->hide = true;
    datas->gameStateMgr->willExit(to);


    RENDERING(game->muteBtn)->hide = RENDERING(game->scorePanel)->hide = TEXT_RENDERING(game->scoreText)->hide = false;
}

bool TutorialStateManager::transitionCanExit(State::Enum to) {

    ADSRComponent* adsr = ADSR(datas->titleGroup);
    adsr->active = false;

    TRANSFORM(datas->titleGroup)->position.Y = adsr->value;

    RENDERING(game->muteBtn)->color.a = (adsr->sustainValue - adsr->value) / (adsr->sustainValue - adsr->idleValue);

    return datas->gameStateMgr->transitionCanExit(to);
}

void TutorialStateManager::exit(State::Enum to) {
    RENDERING(datas->title)->hide = true;

    TEXT_RENDERING(datas->entities.text)->hide = true;
    RENDERING(datas->entities.anim)->hide = true;
    datas->gameStateMgr->exit(to);
    for(std::map<Tutorial::Enum, TutorialStep*>::iterator it=datas->step2mgr.begin();
        it!=datas->step2mgr.end();
        ++it) {
        delete it->second;
    }
    datas->step2mgr.clear();
    BUTTON(game->muteBtn)->enabled = true;
}

