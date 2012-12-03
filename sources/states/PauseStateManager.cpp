/*
 This file is part of Recursive Runner.

 @author Soupe au Caillou - Pierre-Eric Pelloux-Prayer
 @author Soupe au Caillou - Gautier Pelloux-Prayer

 Heriswap is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, version 3.

 Heriswap is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with Heriswap.  If not, see <http://www.gnu.org/licenses/>.
*/
#include "StateManager.h"

struct PauseStateManager::PauseStateManagerDatas {
 
};

PauseStateManager::PauseStateManager(RecursiveRunnerGame* game) : StateManager(State::Pause, game) {
    datas = new PauseStateManagerDatas;
}

PauseStateManager::~PauseStateManager() {
    delete datas;
}

void PauseStateManager::setup() {

}

void PauseStateManager::earlyEnter() {
}

void PauseStateManager::enter() {

}

State::Enum PauseStateManager::update(float dt) {
    return State::Pause;
}

void PauseStateManager::backgroundUpdate(float dt __attribute__((unused))) {

}

void PauseStateManager::exit() {
 
}

void PauseStateManager::lateExit() {

}

bool PauseStateManager::transitionCanExit() {
    return true;
}

bool PauseStateManager::transitionCanEnter() {
    return true;
}