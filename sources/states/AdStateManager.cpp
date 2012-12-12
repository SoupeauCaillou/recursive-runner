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
   int gameb4Ad;
   float lastAdTime;
   float waitAfterAdDisplay;
};

AdStateManager::AdStateManager(RecursiveRunnerGame* game) : StateManager(State::Ad, game) {
   datas = new AdStateManagerDatas;
   datas->gameb4Ad = 1;
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
   // datas->gameb4Ad = game->storageAPI->getGameCountBeforeNextAd();

   LOGI("%s : %d", __PRETTY_FUNCTION__, datas->gameb4Ad);
   if (datas->gameb4Ad > 3) {
      datas->gameb4Ad = 3;
   }

   float timeSinceLAstAd = TimeUtil::getTime() - datas->lastAdTime;

   // postpone ad if previous ad was shown less than 60sec ago
   if (datas->gameb4Ad <= 0 && timeSinceLAstAd < 60) {
      datas->gameb4Ad = 1;
   }

    if (datas->gameb4Ad == 0) {// || timeSinceLAstAd > 150) {
        if (game->adAPI->showAd()) {
            datas->lastAdTime = TimeUtil::getTime();
        } else {
            datas->gameb4Ad = 1;
        }
    }
}


///----------------------------------------------------------------------------//
///--------------------- UPDATE SECTION ---------------------------------------//
///----------------------------------------------------------------------------//
void AdStateManager::backgroundUpdate(float) {
}

State::Enum AdStateManager::update(float) {
    if (datas->gameb4Ad > 0) {
        datas->gameb4Ad--;
        return State::Game;
    } else if(game->adAPI->done()) {
        datas->waitAfterAdDisplay = TimeUtil::getTime();
        return State::Game;
    }
    return State::Ad;
}


///----------------------------------------------------------------------------//
///--------------------- EXIT SECTION -----------------------------------------//
///----------------------------------------------------------------------------//
void AdStateManager::willExit(State::Enum) {
   /*if (datas->gameb4Ad==0)
      datas->gameb4Ad=3;
   datas->gameb4Ad--;
   game->storageAPI->setGameCountBeforeNextAd(datas->gameb4Ad);*/
}

bool AdStateManager::transitionCanExit(State::Enum) {
    if (datas->gameb4Ad == 0) {
        return (TimeUtil::getTime() - datas->waitAfterAdDisplay >= 1.0);
    } else {
        return true;
    }
}

void AdStateManager::exit(State::Enum) {
    if (datas->gameb4Ad == 0) {
        datas->gameb4Ad =3;
    }
}
