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
};

AdStateManager::AdStateManager(RecursiveRunnerGame* game) : StateManager(State::Ad, game) {
   datas = new AdStateManagerDatas;
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
void AdStateManager::willEnter() {
}

bool AdStateManager::transitionCanEnter() {
   return true;
}

void AdStateManager::enter() {
   datas->gameb4Ad = game->storageAPI->getGameCountBeforeNextAd();

   LOGI("%s : %d", __PRETTY_FUNCTION__, datas->gameb4Ad);
   if (datas->gameb4Ad > 3) {
      datas->gameb4Ad = 3;
   }

   float timeSinceLAstAd = TimeUtil::getTime() - datas->lastAdTime;

   // postpone ad if previous ad was shown less than 30sec ago
   if (datas->gameb4Ad <= 0 && timeSinceLAstAd < 30) {
      datas->gameb4Ad = 1;
   }

    if (datas->gameb4Ad==0 || timeSinceLAstAd > 150) {
        if (game->adAPI->showAd()) {
            datas->gameb4Ad = 0;
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
   if (datas->gameb4Ad > 0 || game->adAPI->done()) {
      return State::Game;
   }
   return State::Ad;
}


///----------------------------------------------------------------------------//
///--------------------- EXIT SECTION -----------------------------------------//
///----------------------------------------------------------------------------//
void AdStateManager::willExit() {
   if (datas->gameb4Ad==0)
      datas->gameb4Ad=3;
   datas->gameb4Ad--;
   game->storageAPI->setGameCountBeforeNextAd(datas->gameb4Ad);
}

bool AdStateManager::transitionCanExit() {
   return true;
}

void AdStateManager::exit() {
   game->setupCamera(CameraModeSingle);
}
