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
#include "RunnerSystem.h"
#include "base/MathUtil.h"
#include "systems/TransformationSystem.h"
#include "systems/PhysicsSystem.h"
#include "util/IntersectionUtil.h"
INSTANCE_IMPL(RunnerSystem);
 
RunnerSystem::RunnerSystem() : ComponentSystemImpl<RunnerComponent>("Runner") { 
 
}

void RunnerSystem::DoUpdate(float dt) {
    for(ComponentIt it=components.begin(); it!=components.end(); ++it) {
        Entity a = (*it).first;         
        RunnerComponent* rc = (*it).second;
        
        rc->elapsed += dt;

        if (rc->elapsed >= rc->startTime) {
            TransformationComponent* tc = TRANSFORM(a);
            tc->position.X += rc->speed * dt;
            
            if ((tc->position.X > rc->endPoint.X && rc->speed > 0) || 
                (tc->position.X < rc->endPoint.X && rc->speed < 0)) {
                 std::cout << "finished" << std::endl;
                rc->finished = true;
                tc->position = rc->startPoint;
                rc->elapsed = 0;
                rc->currentJump = 0;
                PhysicsComponent* pc = PHYSICS(a);
                pc->linearVelocity = Vector2::Zero;
                pc->gravity.Y = 0;
            }
        }
        
        if (!rc->jumpTimes.empty() && rc->currentJump < rc->jumpTimes.size()) {
            if (rc->elapsed >= rc->jumpTimes[rc->currentJump]) {
             std::cout << a << " -> jump #" << rc->currentJump << " -> " << rc->jumpTimes[rc->currentJump] << std::endl;
                PhysicsComponent* pc = PHYSICS(a);
                Vector2 force = Vector2(0, 800);
                pc->forces.push_back(std::make_pair(Force(force, Vector2::Zero), 0.016));
                pc->gravity.Y = -30;
                rc->currentJump++;
            }
        }
    }
}

