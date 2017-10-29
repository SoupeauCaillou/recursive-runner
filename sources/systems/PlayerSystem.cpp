/*
    This file is part of RecursiveRunner.

    @author Soupe au Caillou - Jordane Pelloux-Prayer
    @author Soupe au Caillou - Gautier Pelloux-Prayer
    @author Soupe au Caillou - Pierre-Eric Pelloux-Prayer

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
#include "PlayerSystem.h"
#include "RunnerSystem.h"
#include "util/SerializerProperty.h"

INSTANCE_IMPL(PlayerSystem);

PlayerSystem::PlayerSystem() : ComponentSystemImpl<PlayerComponent>(HASH("Player", 0x75a3a9db)) {
    PlayerComponent tc;
    componentSerializer.add(new Property<int>(HASH("points", 0x844cd72f), OFFSET(points, tc)));
    componentSerializer.add(new Property<int>(HASH("runners_count", 0x648f5bb3), OFFSET(runnersCount, tc)));
    componentSerializer.add(new Property<bool>(HASH("ready", 0x744f00e1), OFFSET(ready, tc)));
    componentSerializer.add(new VectorProperty<Color>(HASH("colors", 0xf9ad28ec), OFFSET(colors, tc)));
}

void PlayerSystem::DoUpdate(float) {

}
