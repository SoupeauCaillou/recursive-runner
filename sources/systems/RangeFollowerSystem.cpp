/*
 This file is part of Heriswap.

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
#include "RangeFollowerSystem.h"
#include "systems/TransformationSystem.h"

INSTANCE_IMPL(RangeFollowerSystem);

RangeFollowerSystem::RangeFollowerSystem() : ComponentSystemImpl<RangeFollowerComponent>("RangeFollower") { 
}

void RangeFollowerSystem::DoUpdate(float dt __attribute__((unused))) {
    FOR_EACH_ENTITY_COMPONENT(RangeFollower, a, rc)
        TransformationComponent* tc = TRANSFORM(a);
        if (rc->parent) {
            RangeFollowerComponent* parentRf = RANGE_FOLLOWER(rc->parent);
            float t = parentRf->range.position(TRANSFORM(rc->parent)->position.X);
            tc->position.X = rc->range.lerp(t);
        } else {
            if (tc->position.X < rc->range.t1)
                tc->position.X = rc->range.t1;
            else if (tc->position.X > rc->range.t2)
                tc->position.X = rc->range.t2;
        }
    }
}

