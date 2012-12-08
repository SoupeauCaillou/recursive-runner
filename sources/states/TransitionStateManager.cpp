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

void TransitionStateManager::enter(StateManager* _from, StateManager* _to) {
    from = _from;
    to = _to;
    to->willEnter(_from->state);
}

bool TransitionStateManager::transitionFinished(State::Enum* nextState) {
    if (from->transitionCanExit(to->state) &&
        to->transitionCanEnter(from->state)) {
        *nextState = to->state;
        return true;
    }
    return false;
}

void TransitionStateManager::exit() {
    from->exit(to->state);
}

