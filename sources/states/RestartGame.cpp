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

struct RestartGameState::RestartGameStateDatas {

};

RestartGameState::RestartGameState(RecursiveRunnerGame* game) : StateManager(State::RestartGame, game) {
   datas = new RestartGameStateDatas;
}

RestartGameState::~RestartGameState() {
   delete datas;
}

void RestartGameState::setup() {
   
}


///----------------------------------------------------------------------------//
///--------------------- ENTER SECTION ----------------------------------------//
///----------------------------------------------------------------------------//
void RestartGameState::willEnter(State::Enum ) {
}

bool RestartGameState::transitionCanEnter(State::Enum ) {
   return true;
}

void RestartGameState::enter(State::Enum) {
   
}


///----------------------------------------------------------------------------//
///--------------------- UPDATE SECTION ---------------------------------------//
///----------------------------------------------------------------------------//
void RestartGameState::backgroundUpdate(float) {
}

State::Enum RestartGameState::update(float) {
   return State::Game;
}


///----------------------------------------------------------------------------//
///--------------------- EXIT SECTION -----------------------------------------//
///----------------------------------------------------------------------------//
void RestartGameState::willExit(State::Enum) {

}

bool RestartGameState::transitionCanExit(State::Enum) {
   return true;
}

void RestartGameState::exit(State::Enum) {

}
