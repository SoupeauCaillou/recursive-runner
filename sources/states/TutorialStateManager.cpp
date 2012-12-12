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
#include "systems/TextRenderingSystem.h"
#include "systems/SessionSystem.h"
#include "systems/PhysicsSystem.h"
#include "systems/RunnerSystem.h"

#include "../RecursiveRunnerGame.h"

namespace Tutorial {
    enum Enum {
        IntroduceHero = 0,
        SmallJump,
        ScorePoints,
        BigJump,
        NewHero,
        MeetYourself,
        AvoidYourself,
        TheEnd
    };
}

struct TutorialEntities {
    Entity text;
    Entity anim;
};

struct TutorialStep {
        virtual void enter(SessionComponent* sc, TutorialEntities* entities) = 0;
        virtual bool mustUpdateGame(SessionComponent* sc, TutorialEntities* entities) = 0;
        virtual bool canExit(SessionComponent* sc, TutorialEntities* entities) = 0;
};



#include "tutorial/IntroduceHero.h"
#include "tutorial/SmallJump.h"
#include "tutorial/ScorePoints.h"
#include "tutorial/BigJump.h"
#include "tutorial/NewHero.h"
#include "tutorial/MeetYourself.h"
#include "tutorial/AvoidYourself.h"
#include "tutorial/TheEnd.h"

struct TutorialStateManager::TutorialStateManagerDatas {
    GameStateManager* gameStateMgr;
    TutorialEntities entities;
    Tutorial::Enum currentStep;
    std::map<Tutorial::Enum, TutorialStep*> step2mgr;
};

TutorialStateManager::TutorialStateManager(RecursiveRunnerGame* game) : StateManager(State::Tutorial, game) {
   datas = new TutorialStateManagerDatas;
   datas->gameStateMgr = new GameStateManager(game);
   datas->step2mgr[Tutorial::IntroduceHero] = new IntroduceHeroTutorialStep;
   datas->step2mgr[Tutorial::SmallJump] = new SmallJumpTutorialStep;
   datas->step2mgr[Tutorial::ScorePoints] = new ScorePointsTutorialStep;
   datas->step2mgr[Tutorial::BigJump] = new BigJumpTutorialStep;
   datas->step2mgr[Tutorial::NewHero] = new NewHeroTutorialStep;
   datas->step2mgr[Tutorial::MeetYourself] = new MeetYourselfTutorialStep;
   datas->step2mgr[Tutorial::AvoidYourself] = new AvoidYourselfTutorialStep;
   datas->step2mgr[Tutorial::TheEnd] = new TheEndTutorialStep;
}

TutorialStateManager::~TutorialStateManager() {
    delete datas->gameStateMgr;
    for(std::map<Tutorial::Enum, TutorialStep*>::iterator it=datas->step2mgr.begin();
        it!=datas->step2mgr.end();
        ++it) {
        delete it->second;
    }
    delete datas;
}

void TutorialStateManager::setup() {
    // setup game
    datas->gameStateMgr->setup();
    // setup tutorial
    Entity text = datas->entities.text = theEntityManager.CreateEntity();
    ADD_COMPONENT(text, Transformation);
    ADD_COMPONENT(text, TextRendering);
    TEXT_RENDERING(text)->charHeight = PlacementHelper::GimpHeightToScreen(45);
    TEXT_RENDERING(text)->hide = true;
    TEXT_RENDERING(text)->cameraBitMask = 0x3;
    TEXT_RENDERING(text)->color = Color(40.0 / 255, 32.0/255, 30.0/255, 0.8);

    Entity anim = datas->entities.anim = theEntityManager.CreateEntity();
    ADD_COMPONENT(anim, Transformation);
    ADD_COMPONENT(anim, Rendering);
    RENDERING(anim)->cameraBitMask = 0x3;
    ADD_COMPONENT(anim, Animation);
}


///----------------------------------------------------------------------------//
///--------------------- ENTER SECTION ----------------------------------------//
///----------------------------------------------------------------------------//
void TutorialStateManager::willEnter(State::Enum from) {
    datas->gameStateMgr->willEnter(from);

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
}

bool TutorialStateManager::transitionCanEnter(State::Enum from) {
   bool gameCanEnter = datas->gameStateMgr->transitionCanEnter(from);
   
   return gameCanEnter;
}

void TutorialStateManager::enter(State::Enum from) {
    SessionComponent* session = SESSION(theSessionSystem.RetrieveAllEntityWithComponent().front());
    session->userInputEnabled = false;

    datas->gameStateMgr->enter(from);
    datas->currentStep = Tutorial::IntroduceHero;
    datas->step2mgr[datas->currentStep]->enter(session, &datas->entities);
}


///----------------------------------------------------------------------------//
///--------------------- UPDATE SECTION ---------------------------------------//
///----------------------------------------------------------------------------//
void TutorialStateManager::backgroundUpdate(float) {
}

State::Enum TutorialStateManager::update(float dt) {
    SessionComponent* session = SESSION(theSessionSystem.RetrieveAllEntityWithComponent().front());

    if (datas->step2mgr[datas->currentStep]->mustUpdateGame(session, &datas->entities)) {
        datas->gameStateMgr->update(dt);
    }
    if (datas->step2mgr[datas->currentStep]->canExit(session, &datas->entities)) {
        if (datas->currentStep == Tutorial::TheEnd) {
            return State::Menu;
        } else {
            datas->currentStep = (Tutorial::Enum) (datas->currentStep + 1);
            datas->step2mgr[datas->currentStep]->enter(session, &datas->entities);
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
    datas->gameStateMgr->willExit(to);
}

bool TutorialStateManager::transitionCanExit(State::Enum to) {
   return datas->gameStateMgr->transitionCanExit(to);
}

void TutorialStateManager::exit(State::Enum to) {
    datas->gameStateMgr->exit(to);
}

