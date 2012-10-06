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
#include "base/EntityManager.h"
#include "systems/TransformationSystem.h"
#include "systems/PhysicsSystem.h"
#include "systems/RenderingSystem.h"
#include "systems/AnimationSystem.h"
#include "systems/AutoDestroySystem.h"
#include "util/IntersectionUtil.h"

INSTANCE_IMPL(RunnerSystem);
 
const float MinJumpDuration = 0.016;
float MaxJumpDuration = 0.15;

RunnerSystem::RunnerSystem() : ComponentSystemImpl<RunnerComponent>("Runner") { 
    RunnerComponent tc;
#ifdef SAC_NETWORK
    componentSerializer.add(new EntityProperty(OFFSET(playerOwner, tc)));
#endif
    componentSerializer.add(new EpsilonProperty<float>(OFFSET(startPoint.X, tc), 0.001));
    componentSerializer.add(new EpsilonProperty<float>(OFFSET(startPoint.Y, tc), 0.001));
    componentSerializer.add(new EpsilonProperty<float>(OFFSET(endPoint.X, tc), 0.001));
    componentSerializer.add(new EpsilonProperty<float>(OFFSET(endPoint.Y, tc), 0.001));
    componentSerializer.add(new EpsilonProperty<float>(OFFSET(maxSpeed, tc), 0.001));
    componentSerializer.add(new EpsilonProperty<float>(OFFSET(speed, tc), 0.001));
    componentSerializer.add(new Property(OFFSET(finished, tc), sizeof(bool)));
    componentSerializer.add(new Property(OFFSET(ghost, tc), sizeof(bool)));
    componentSerializer.add(new Property(OFFSET(killed, tc), sizeof(bool)));
    componentSerializer.add(new Property(OFFSET(currentJump, tc), sizeof(bool)));
    componentSerializer.add(new VectorProperty<float>(OFFSET(jumpTimes, tc)));
    componentSerializer.add(new VectorProperty<float>(OFFSET(jumpDurations, tc)));
    componentSerializer.add(new VectorProperty<float>(OFFSET(coins, tc)));
}

static void killRunner(Entity runner) {
    RENDERING(runner)->hide = true;
    RENDERING(runner)->color = Color(0,0,1);
    int dir = 1 - 2 * MathUtil::RandomIntInRange(0, 2);//(RUNNER(current)->speed > 0) ? 1 : -1;
    Entity e = theEntityManager.CreateEntity();
    ADD_COMPONENT(e, Transformation);
    *TRANSFORM(e) = *TRANSFORM(runner);
    ADD_COMPONENT(e, Rendering);
    RENDERING(e)->hide = false;
    RENDERING(e)->cameraBitMask = (0x3 << 1);
    ADD_COMPONENT(e, Animation);
    ANIMATION(e)->name = (dir > 0) ? "flyL2R" : "flyR2L";
    ADD_COMPONENT(e, Physics);
    PHYSICS(e)->mass = 1;
    PHYSICS(e)->gravity.Y = -10;
    PHYSICS(e)->forces.push_back(std::make_pair(
    Force(Vector2::Rotate(Vector2(MathUtil::RandomIntInRange(500, 700), 0), dir > 0 ? MathUtil::RandomFloatInRange(0.25, 1.5) : MathUtil::RandomFloatInRange(1.5, 3.14-0.25)),
        Vector2::Rotate(TRANSFORM(e)->size * 0.2, MathUtil::RandomFloat(6.28))), 0.016));
    ADD_COMPONENT(e, AutoDestroy);
    AUTO_DESTROY(e)->type = AutoDestroyComponent::OUT_OF_SCREEN;
}

void RunnerSystem::DoUpdate(float dt) {
    std::vector<Entity> killedRunners;
    for(ComponentIt it=components.begin(); it!=components.end(); ++it) {
        Entity a = (*it).first;         
        RunnerComponent* rc = (*it).second;
        PhysicsComponent* pc = PHYSICS(a);

        if (rc->killed) {
            if (rc->elapsed >= 0) {
                killRunner(a);
                killedRunners.push_back(a);
            }
            rc->elapsed = -1;
            continue;
        }

        rc->elapsed += dt;
        
        if (rc->elapsed >= rc->startTime) {
            TransformationComponent* tc = TRANSFORM(a);
            /*
            if (pc->gravity.Y < 0) {
                rc->speed -= rc->speed * dt * 0.3;
            } else if (rc->maxSpeed < 0) {
                rc->speed = MathUtil::Max(rc->maxSpeed,
                    rc->speed + rc->speed * dt * 4.f);
            } else {
                rc->speed = MathUtil::Min(rc->maxSpeed,
                    rc->speed + rc->speed * dt * 4.f);
            }*/
            
            tc->position.X += rc->speed * dt;
            
            if ((tc->position.X > rc->endPoint.X && rc->speed > 0) || 
                (tc->position.X < rc->endPoint.X && rc->speed < 0)) {
                 std::cout << a << " finished!" << std::endl;
                rc->finished = true;
                rc->oldNessBonus++;
                rc->coinSequenceBonus = 1;
                rc->ghost = true;
                tc->position = rc->startPoint;
                rc->elapsed = rc->jumpingSince = 0;
                rc->currentJump = 0;
                
                pc->linearVelocity = Vector2::Zero;
                pc->gravity.Y = 0;
                rc->coins.clear();
            }
        }
        
        if (!rc->jumpTimes.empty() && rc->currentJump < rc->jumpTimes.size()) {
            if (rc->elapsed >= rc->jumpTimes[rc->currentJump] && rc->jumpingSince == 0) {           
                // std::cout << a << " -> jump #" << rc->currentJump << " -> " << rc->jumpTimes[rc->currentJump] << std::endl;
                Vector2 force = Vector2(0, 900);
                pc->forces.push_back(std::make_pair(Force(force, Vector2::Zero), 0.016));
                rc->jumpingSince = 0.001;
                
                ANIMATION(a)->name = (rc->speed > 0) ? "jumpL2R" : "jumpR2L";
            } else {
                if (rc->jumpingSince > 0) {
                    rc->jumpingSince += dt;
                    if (rc->jumpingSince > rc->jumpDurations[rc->currentJump] && rc->jumpingSince >= MinJumpDuration) {
                        
                        pc->gravity.Y = -100;
                        rc->jumpingSince = 0;
                        rc->currentJump++;
                    }
                }
            }
        }
    }

    if (!killedRunners.empty()) {
        for (int i=0;i<killedRunners.size(); i++) {
            Entity a = killedRunners[i];
            int bonus = RUNNER(a)->oldNessBonus;
            for(ComponentIt it=components.begin(); it!=components.end(); ++it) {
                if (it->first == a)
                    continue;
                RunnerComponent* rc = (*it).second;
                if (bonus < rc->oldNessBonus) {
                    std::cout << rc->oldNessBonus << " - " << killedRunners.size() << std::endl;
                    rc->oldNessBonus--;
                    assert(rc->oldNessBonus >= 0);
                }
            }
            theEntityManager.DeleteEntity(a);
        }
    }
}

