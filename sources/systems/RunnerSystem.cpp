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
#include "RunnerSystem.h"
#include "base/MathUtil.h"
#include "base/EntityManager.h"
#include "systems/TransformationSystem.h"
#include "systems/PhysicsSystem.h"
#include "systems/RenderingSystem.h"
#include "systems/AnimationSystem.h"
#include "systems/AutoDestroySystem.h"
#include "util/IntersectionUtil.h"
std::map<TextureRef, CollisionZone> texture2Collision;

INSTANCE_IMPL(RunnerSystem);

float RunnerSystem::MinJumpDuration = 0.005;
float RunnerSystem::MaxJumpDuration = 0.2;

RunnerSystem::RunnerSystem() : ComponentSystemImpl<RunnerComponent>("Runner") {
    RunnerComponent tc;
    componentSerializer.add(new EntityProperty(OFFSET(playerOwner, tc)));
    componentSerializer.add(new EntityProperty(OFFSET(collisionZone, tc)));
    componentSerializer.add(new EpsilonProperty<float>(OFFSET(startPoint.X, tc), 0.001));
    componentSerializer.add(new EpsilonProperty<float>(OFFSET(startPoint.Y, tc), 0.001));
    componentSerializer.add(new EpsilonProperty<float>(OFFSET(endPoint.X, tc), 0.001));
    componentSerializer.add(new EpsilonProperty<float>(OFFSET(endPoint.Y, tc), 0.001));
    componentSerializer.add(new Property(OFFSET(color, tc), sizeof(Color)));
    componentSerializer.add(new EpsilonProperty<float>(OFFSET(speed, tc), 0.001));
    componentSerializer.add(new Property(OFFSET(finished, tc), sizeof(bool)));
    componentSerializer.add(new Property(OFFSET(ghost, tc), sizeof(bool)));
    componentSerializer.add(new Property(OFFSET(killed, tc), sizeof(bool)));
    componentSerializer.add(new EpsilonProperty<float>(OFFSET(elapsed, tc), 0.001));
    componentSerializer.add(new EpsilonProperty<float>(OFFSET(jumpingSince, tc), 0.001));
    componentSerializer.add(new Property(OFFSET(currentJump, tc), sizeof(bool)));
    componentSerializer.add(new Property(OFFSET(oldNessBonus, tc), sizeof(int)));
    componentSerializer.add(new Property(OFFSET(coinSequenceBonus, tc), sizeof(int)));
    componentSerializer.add(new VectorProperty<float>(OFFSET(jumpTimes, tc)));
    componentSerializer.add(new VectorProperty<float>(OFFSET(jumpDurations, tc)));
    componentSerializer.add(new VectorProperty<float>(OFFSET(coins, tc)));
}

static void killRunner(Entity runner) {
    RENDERING(runner)->hide = true;
    int dir = 1 - 2 * MathUtil::RandomIntInRange(0, 2);//(RUNNER(current)->speed > 0) ? 1 : -1;
    Entity e = theEntityManager.CreateEntity();
    ADD_COMPONENT(e, Transformation);
    *TRANSFORM(e) = *TRANSFORM(runner);
    ADD_COMPONENT(e, Rendering);
    RENDERING(e)->hide = false;
    RENDERING(e)->color = RENDERING(runner)->color;
    RENDERING(e)->texture = RENDERING(runner)->texture;
    RENDERING(e)->cameraBitMask = (0x3 << 1);
    ADD_COMPONENT(e, Animation);
    ANIMATION(e)->name = "runL2R";
    ADD_COMPONENT(e, Physics);
    PHYSICS(e)->mass = 1;
    PHYSICS(e)->gravity.Y = -10;
    PHYSICS(e)->forces.push_back(std::make_pair(
    Force(Vector2::Rotate(Vector2(MathUtil::RandomIntInRange(500, 700), 0), dir > 0 ? MathUtil::RandomFloatInRange(0.25, 1.5) : MathUtil::RandomFloatInRange(1.5, 3.14-0.25)),
        Vector2::Rotate(TRANSFORM(e)->size * 0.2, MathUtil::RandomFloat(6.28))), 0.016));
    ADD_COMPONENT(e, AutoDestroy);
    AUTO_DESTROY(e)->type = AutoDestroyComponent::LIFETIME;
    AUTO_DESTROY(e)->params.lifetime.map2AlphaRendering = true;
    AUTO_DESTROY(e)->params.lifetime.value = 0.5;
}

void RunnerSystem::DoUpdate(float dt) {
    std::vector<Entity> killedRunners;
    FOR_EACH_ENTITY_COMPONENT(Runner, a, rc)
        PhysicsComponent* pc = PHYSICS(a);
        pc->mass = 1;
        TransformationComponent* tc = TRANSFORM(a);
        {
            RenderingComponent* rendc = RENDERING(a);
            TransformationComponent* ttt = TRANSFORM(rc->collisionZone);
            const CollisionZone& cz = texture2Collision[rendc->texture];
            ttt->position = tc->size * cz.position;
            ttt->size = tc->size * cz.size;
            ttt->rotation = cz.rotation;
            if (rendc->mirrorH) {
                ttt->position.X = -ttt->position.X;
                ttt->rotation = -ttt->rotation;
            }
        }
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
            tc->position.X += rc->speed * dt;

            if ((tc->position.X > rc->endPoint.X && rc->speed > 0) ||
                (tc->position.X < rc->endPoint.X && rc->speed < 0)) {
                if (!rc->ghost)
                    std::cout << a << " finished! (" << rc->coins.size() << ")" << TimeUtil::getTime() << " (pos=" << tc->position << ") "<< rc->endPoint.X << std::endl;
                ANIMATION(a)->name = "runL2R";
                rc->finished = true;
                rc->oldNessBonus++;
                rc->coinSequenceBonus = 1;
                rc->ghost = true;
                rc->startTime = MathUtil::RandomFloatInRange(0, 2.0f);
                RENDERING(a)->color = Color(27.0/255, 2.0/255, 2.0/255, 0.8);
                tc->position = rc->startPoint;
                rc->elapsed = rc->jumpingSince = 0;
                rc->currentJump = 0;

                pc->linearVelocity = Vector2::Zero;
                pc->gravity.Y = 0;
                rc->coins.clear();
            }
        }

        if (!rc->jumpTimes.empty() && rc->currentJump < (int)rc->jumpTimes.size()) {
            if ((rc->elapsed - rc->startTime)>= rc->jumpTimes[rc->currentJump] && rc->jumpingSince == 0) {
                // std::cout << a << " -> jump #" << rc->currentJump << " -> " << rc->jumpTimes[rc->currentJump] << std::endl;
                Vector2 force = Vector2(0, 1800 * 1.5);
                pc->forces.push_back(std::make_pair(Force(force, Vector2::Zero), RunnerSystem::MinJumpDuration));
                rc->jumpingSince = 0.001;
                pc->gravity.Y = -50;
                ANIMATION(a)->name = "jumpL2R_up";
                RENDERING(a)->mirrorH = (rc->speed < 0);
            } else {
                if (rc->jumpingSince > 0) {
                    rc->jumpingSince += dt;
                    if (rc->jumpingSince > rc->jumpDurations[rc->currentJump]) {// && rc->jumpingSince >= MinJumpDuration) {
                        //ANIMATION(a)->name = (rc->speed > 0) ? "jumpL2R_down" : "jumpR2L_down";
                        pc->gravity.Y = -150;
                        rc->jumpingSince = 0;
                        rc->currentJump++;
                        } else {
                        Vector2 force = Vector2(0, 100 * 1);
                        pc->forces.push_back(std::make_pair(Force(force, Vector2::Zero), dt));
                    }
                }
            }
        }
        if (pc->gravity.Y < 0 && pc->linearVelocity.Y < -10) {
            ANIMATION(a)->name = "jumpL2R_down";
            RENDERING(a)->mirrorH = (rc->speed < 0);
        }
             /*RENDERING(a)->texture = InvalidTextureRef;
            ANIMATION(a)->name = "";*/
    }

    if (!killedRunners.empty()) {
        for (unsigned i=0;i<killedRunners.size(); i++) {
            Entity a = killedRunners[i];
            int bonus = RUNNER(a)->oldNessBonus;
            FOR_EACH_ENTITY_COMPONENT(Runner, b, rc)
                if (b == a)
                    continue;
                if (bonus < rc->oldNessBonus) {
                    rc->oldNessBonus--;
                    assert(rc->oldNessBonus >= 0);
                }
            }
            theEntityManager.DeleteEntity(RUNNER(a)->collisionZone);
            theEntityManager.DeleteEntity(a);
        }
    }
}

