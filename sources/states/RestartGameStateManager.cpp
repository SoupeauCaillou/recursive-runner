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

struct RestartGameStateManager::RestartGameStateManagerDatas {

};

RestartGameStateManager::RestartGameStateManager(RecursiveRunnerGame* game) : StateManager(State::Ad, game) {
   datas = new RestartGameStateManagerDatas;
}

RestartGameStateManager::~RestartGameStateManager() {
   delete datas;
}

void RestartGameStateManager::setup() {
   
}


///----------------------------------------------------------------------------//
///--------------------- ENTER SECTION ----------------------------------------//
///----------------------------------------------------------------------------//
void RestartGameStateManager::willEnter(State::Enum ) {
}

bool RestartGameStateManager::transitionCanEnter(State::Enum ) {
   return true;
}

void RestartGameStateManager::enter(State::Enum) {
   
}


///----------------------------------------------------------------------------//
///--------------------- UPDATE SECTION ---------------------------------------//
///----------------------------------------------------------------------------//
void RestartGameStateManager::backgroundUpdate(float) {
}

State::Enum RestartGameStateManager::update(float) {
   return State::Game;
}


///----------------------------------------------------------------------------//
///--------------------- EXIT SECTION -----------------------------------------//
///----------------------------------------------------------------------------//
void RestartGameStateManager::willExit(State::Enum) {

}

bool RestartGameStateManager::transitionCanExit(State::Enum) {
   return true;
}

void RestartGameStateManager::exit(State::Enum) {

}
