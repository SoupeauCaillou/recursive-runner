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
#include "RangeFollowerSystem.h"
#include "systems/TransformationSystem.h"
#include "util/SerializerProperty.h"

INSTANCE_IMPL(RangeFollowerSystem);

RangeFollowerSystem::RangeFollowerSystem() : ComponentSystemImpl<RangeFollowerComponent>(HASH("RangeFollower", 0x63b75580)) {
    RangeFollowerComponent tc;
    componentSerializer.add(new EntityProperty(HASH("parent", 0x7ae3b713), OFFSET(parent, tc)));
    componentSerializer.add(new IntervalProperty<float>(HASH("range", 0x85dd6984), OFFSET(range, tc)));
}

void RangeFollowerSystem::DoUpdate(float) {
    FOR_EACH_ENTITY_COMPONENT(RangeFollower, a, rc)
        TransformationComponent* tc = TRANSFORM(a);
        if (rc->parent) {
            RangeFollowerComponent* parentRf = RANGE_FOLLOWER(rc->parent);
            float t = parentRf->range.position(TRANSFORM(rc->parent)->position.x);
            tc->position.x = rc->range.lerp(t);
        } else {
            if (tc->position.x < rc->range.t1)
                tc->position.x = rc->range.t1;
            else if (tc->position.x > rc->range.t2)
                tc->position.x = rc->range.t2;
        }
    }
}
