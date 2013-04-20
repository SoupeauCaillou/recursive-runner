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
#include "SessionSystem.h"

INSTANCE_IMPL(SessionSystem);

SessionSystem::SessionSystem() : ComponentSystemImpl<SessionComponent>("Session") {
    SessionComponent tc;
    componentSerializer.add(new Property<int>("numPlayers", OFFSET(numPlayers, tc)));
    componentSerializer.add(new EntityProperty("currentRunner", OFFSET(currentRunner, tc)));
    componentSerializer.add(new Property<bool>("userInputEnabled", OFFSET(userInputEnabled, tc)));
    componentSerializer.add(new VectorProperty<Entity>("runners", OFFSET(runners, tc)));
    componentSerializer.add(new VectorProperty<Entity>("coins", OFFSET(coins, tc)));
    componentSerializer.add(new VectorProperty<Entity>("players", OFFSET(players, tc)));
    componentSerializer.add(new VectorProperty<Entity>("links", OFFSET(links, tc)));
    componentSerializer.add(new VectorProperty<Entity>("sparkling", OFFSET(sparkling, tc)));
}

void SessionSystem::DoUpdate(float) {
    // nothing
}

#if SAC_INGAME_EDITORS
void SessionSystem::addEntityPropertiesToBar(unsigned long, CTwBar*) {}
#endif
