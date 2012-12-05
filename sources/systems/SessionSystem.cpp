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
#include "SessionSystem.h"

INSTANCE_IMPL(SessionSystem);

SessionSystem::SessionSystem() : ComponentSystemImpl<SessionComponent>("Session") { 
    SessionComponent tc;
    componentSerializer.add(new Property(OFFSET(numPlayers, tc), sizeof(int)));
    componentSerializer.add(new EntityProperty(OFFSET(currentRunner, tc)));
    componentSerializer.add(new VectorProperty<Entity>(OFFSET(runners, tc)));
    componentSerializer.add(new VectorProperty<Entity>(OFFSET(coins, tc)));
    componentSerializer.add(new VectorProperty<Entity>(OFFSET(players, tc)));
    componentSerializer.add(new VectorProperty<Entity>(OFFSET(links, tc)));
    componentSerializer.add(new VectorProperty<Entity>(OFFSET(sparkling, tc)));
}

void SessionSystem::DoUpdate(float dt) {
    // nothing
}

