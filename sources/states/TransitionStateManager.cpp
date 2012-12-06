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

struct TransitionStateManager::TransitionStateManagerDatas {
    State::Enum state;
    StateManager* from, *to;
};

TransitionStateManager::TransitionStateManager(State::Enum state, StateManager* from, StateManager* to, RecursiveRunnerGame* game) : StateManager(state, game) {
    datas = new TransitionStateManagerDatas;
    datas->state = state;
    datas->from = from;
    datas->to = to;
}

TransitionStateManager::~TransitionStateManager() {
    delete datas;
}

void TransitionStateManager::setup() {

}

void TransitionStateManager::enter() {
    datas->to->earlyEnter();
}

State::Enum TransitionStateManager::update(float dt __attribute__((unused))) {
    if (datas->from->transitionCanExit() &&
        datas->to->transitionCanEnter()) {
        return datas->to->state;
    }
    return state;
}

void TransitionStateManager::exit() {
    datas->from->lateExit();
}

