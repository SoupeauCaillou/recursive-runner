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

#include "systems/TransformationSystem.h"
#include "systems/RenderingSystem.h"
#include "systems/TextRenderingSystem.h"
#include "systems/ButtonSystem.h"

#include "../RecursiveRunnerGame.h"

struct AdStateManager::AdStateManagerDatas {
   int gamesb4Ad;
   float lastAdTime;
   float waitAfterAdDisplay;
};

AdStateManager::AdStateManager(RecursiveRunnerGame* game) : StateManager(State::Ad, game) {
   datas = new AdStateManagerDatas;
   datas->gamesb4Ad = 1;
}

AdStateManager::~AdStateManager() {
   delete datas;
}

void AdStateManager::setup() {
   datas->lastAdTime = -30.;
}


///----------------------------------------------------------------------------//
///--------------------- ENTER SECTION ----------------------------------------//
///----------------------------------------------------------------------------//
void AdStateManager::willEnter(State::Enum ) {
}

bool AdStateManager::transitionCanEnter(State::Enum ) {
   return true;
}

void AdStateManager::enter(State::Enum) {
   LOGI(datas->gamesb4Ad << " game(s) left")

   float timeSinceLAstAd = TimeUtil::GetTime() - datas->lastAdTime;

   //anticheating?
   if (datas->gamesb4Ad > 3) {
      datas->gamesb4Ad = 3;
   }


   // postpone ad if previous ad was shown less than 60sec ago
   else if (/*false && */datas->gamesb4Ad <= 0 && timeSinceLAstAd < 60) {
      datas->gamesb4Ad = 1;
   }

   // must show an ad
   else if (datas->gamesb4Ad == 0) {
       //try to launch the ad
        if (game->gameThreadContext->adAPI->showAd()) {
            datas->lastAdTime = TimeUtil::GetTime();
        //if not ready, retry next game
        } else {
           LOGI("no ad ready, will retry next game!");
            datas->gamesb4Ad = 1;
        }
    }
}


///----------------------------------------------------------------------------//
///--------------------- UPDATE SECTION ---------------------------------------//
///----------------------------------------------------------------------------//
void AdStateManager::backgroundUpdate(float) {
}

State::Enum AdStateManager::update(float) {
    if (datas->gamesb4Ad > 0) {
        return State::Game;
    } else if(game->gameThreadContext->adAPI->done()) {
        datas->waitAfterAdDisplay = TimeUtil::GetTime();
        return State::Game;
    }
    return State::Ad;
}


///----------------------------------------------------------------------------//
///--------------------- EXIT SECTION -----------------------------------------//
///----------------------------------------------------------------------------//
void AdStateManager::willExit(State::Enum) {
}

bool AdStateManager::transitionCanExit(State::Enum) {
    if (datas->gamesb4Ad == 0) {
       //wait 1 sec after the ad was showed
        return (TimeUtil::GetTime() - datas->waitAfterAdDisplay >= 1.0);
    } else {
        return true;
    }
}

void AdStateManager::exit(State::Enum) {
   //decrease gamesb4Ad (-=1 if >0 else =3)
    datas->gamesb4Ad = (datas->gamesb4Ad + 3) % 4;
}
