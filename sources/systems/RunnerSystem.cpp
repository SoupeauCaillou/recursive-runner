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
#include "RunnerSystem.h"
#include "base/EntityManager.h"
#include "systems/TransformationSystem.h"
#include "systems/PhysicsSystem.h"
#include "systems/RenderingSystem.h"
#include "systems/AnimationSystem.h"
#include "systems/AnchorSystem.h"
#include "systems/AutoDestroySystem.h"
#include "util/IntersectionUtil.h"

#include "../RecursiveRunnerGame.h"
std::map<TextureRef, CollisionZone> texture2Collision;

INSTANCE_IMPL(RunnerSystem);

float RunnerSystem::MinJumpDuration = 0.005;
float RunnerSystem::MaxJumpDuration = 0.2;

RunnerSystem::RunnerSystem() : ComponentSystemImpl<RunnerComponent>(HASH("Runner", 0xe5dc730a)) {
    RunnerComponent tc;
    componentSerializer.add(new EntityProperty(HASH("player_owner", 0xd5181aa0), OFFSET(playerOwner, tc)));
    componentSerializer.add(new EntityProperty(HASH("collision_zone", 0x2a513634), OFFSET(collisionZone, tc)));
    componentSerializer.add(new Property<glm::vec2>(HASH("start_point", 0xdd8c9350), OFFSET(startPoint.x, tc), glm::vec2(0.001, 0)));
    componentSerializer.add(new Property<glm::vec2>(HASH("end_point", 0x85fa62a4), OFFSET(endPoint, tc), glm::vec2(0.001, 0)));
    componentSerializer.add(new Property<Color>(HASH("color", 0xccc35cf8), OFFSET(color, tc)));
    componentSerializer.add(new Property<float>(HASH("speed", 0x9e8699c2), OFFSET(speed, tc), 0.001));
    componentSerializer.add(new Property<bool>(HASH("finished", 0x8f15805b), OFFSET(finished, tc)));
    componentSerializer.add(new Property<bool>(HASH("ghost", 0x72965f98), OFFSET(ghost, tc)));
    componentSerializer.add(new Property<bool>(HASH("killed", 0xe805462a), OFFSET(killed, tc)));
    componentSerializer.add(new Property<float>(HASH("elapsed", 0x621cadb3), OFFSET(elapsed, tc), 0.001));
    componentSerializer.add(new Property<float>(HASH("jumping_since", 0xe830c835), OFFSET(jumpingSince, tc), 0.001));
    componentSerializer.add(new Property<bool>(HASH("current_jump", 0x9bb0b842), OFFSET(currentJump, tc)));
    componentSerializer.add(new Property<int>(HASH("oldness_bonus", 0x14ad4fd5), OFFSET(oldNessBonus, tc)));
    componentSerializer.add(new Property<int>(HASH("coin_sequence_bonus", 0xe7d65188), OFFSET(coinSequenceBonus, tc)));
    componentSerializer.add(new VectorProperty<float>(HASH("jump_times", 0xf291681a), OFFSET(jumpTimes, tc)));
    componentSerializer.add(new VectorProperty<float>(HASH("jump_durations", 0xa9048c79), OFFSET(jumpDurations, tc)));
    componentSerializer.add(new Property<int>(HASH("total_coins_earned", 0x7852232e), OFFSET(totalCoinsEarned, tc)));
    componentSerializer.add(new VectorProperty<float>(HASH("coins", 0xb2cf216c), OFFSET(coins, tc)));
}

static void killRunner(Entity runner) {
    RENDERING(runner)->show = false;
    Entity e = theEntityManager.CreateEntityFromTemplate("ingame/kill_runner_anim");
    *TRANSFORM(e) = *TRANSFORM(runner);
    TRANSFORM(e)->position.y += TRANSFORM(e)->size.y * 0.1;
    RENDERING(e)->texture = RENDERING(runner)->texture;
    RENDERING(e)->color.a = 0.5;
}

void RunnerSystem::DoUpdate(float dt) {
    std::vector<Entity> killedRunners;
    FOR_EACH_ENTITY_COMPONENT(Runner, a, rc)
        PhysicsComponent* pc = PHYSICS(a);
        pc->mass = 1;
        TransformationComponent* tc = TRANSFORM(a);
        {
            RenderingComponent* rendc = RENDERING(a);
            auto* tta = ANCHOR(rc->collisionZone);
            auto* ttt = TRANSFORM(rc->collisionZone);
            const CollisionZone& cz = texture2Collision[rendc->texture];
            tta->position = tc->size * cz.position;
            ttt->size = tc->size * cz.size;
            tta->rotation = cz.rotation;
            if (rendc->flags & RenderingFlags::MirrorHorizontal) {
                ttt->position.x = -ttt->position.x;
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
            tc->position.x += rc->speed * dt;

            if ((tc->position.x > rc->endPoint.x && rc->speed > 0) ||
                (tc->position.x < rc->endPoint.x && rc->speed < 0)) {
                if (!rc->ghost)
                    LOGV(1, a << " finished! (" << rc->coins.size() << ") (pos=" << tc->position
                        << ") "<< rc->endPoint.x);
                ANIMATION(a)->name = HASH("runL2R", 0xda1d330c);
                rc->finished = true;
                rc->oldNessBonus++;
                rc->coinSequenceBonus = 1;
                rc->ghost = true;
                LOGF_IF(RecursiveRunnerGame::nextRunnerStartTimeIndex >= 100, "Not enough start times");
                rc->startTime = RecursiveRunnerGame::nextRunnerStartTime[RecursiveRunnerGame::nextRunnerStartTimeIndex++];
                RENDERING(a)->color = Color(27.0/255, 2.0/255, 2.0/255, 0.8);
                tc->position = rc->startPoint;
                rc->elapsed = rc->jumpingSince = 0;
                rc->currentJump = 0;

                pc->linearVelocity =  glm::vec2(0.0f);
                pc->gravity.y = 0;
                rc->totalCoinsEarned = rc->coins.size();
                rc->coins.clear();
            }
        }

        if (!rc->jumpTimes.empty() && rc->currentJump < (int)rc->jumpTimes.size()) {
            if ((rc->elapsed - rc->startTime)>= rc->jumpTimes[rc->currentJump] && rc->jumpingSince == 0) {
                // std::cout << a << " -> jump #" << rc->currentJump << " -> " << rc->jumpTimes[rc->currentJump] << std::endl;
                glm::vec2 force(0, 1800 * 1.5);
                pc->forces.push_back(std::make_pair(Force(force,  glm::vec2(0.0f)), RunnerSystem::MinJumpDuration));
                rc->jumpingSince = 0.001;
                pc->gravity.y = -50;
                ANIMATION(a)->name = HASH("jumpL2R_up", 0xc043b37b);
                if (rc->speed < 0)
                    RENDERING(a)->flags |= RenderingFlags::MirrorHorizontal;
                else
                    RENDERING(a)->flags &= ~(RenderingFlags::MirrorHorizontal);
            } else {
                if (rc->jumpingSince > 0) {
                    rc->jumpingSince += dt;
                    if (rc->jumpingSince > rc->jumpDurations[rc->currentJump]) {// && rc->jumpingSince >= MinJumpDuration) {
                        //ANIMATION(a)->name = (rc->speed > 0) ? "jumpL2R_down" : "jumpR2L_down";
                        pc->gravity.y = -150;
                        rc->jumpingSince = 0;
                        rc->currentJump++;
                        } else {
                         glm::vec2 force(0, 100 * 1);
                        pc->forces.push_back(std::make_pair(Force(force,  glm::vec2(0.0f)), dt));
                    }
                }
            }
        }
        if (pc->gravity.y < 0 && pc->linearVelocity.y < -10) {
            ANIMATION(a)->name = HASH("jumpL2R_down", 0xc810b848);
            if (rc->speed < 0)
                RENDERING(a)->flags |= RenderingFlags::MirrorHorizontal;
            else
                RENDERING(a)->flags &= ~(RenderingFlags::MirrorHorizontal);
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
