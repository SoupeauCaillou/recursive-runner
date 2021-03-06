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
#include "PlatformerSystem.h"
#include "systems/TransformationSystem.h"
#include "systems/PhysicsSystem.h"
#include "systems/AnimationSystem.h"
#include "systems/RenderingSystem.h"
#include "systems/RunnerSystem.h"
#include "util/IntersectionUtil.h"
#include "util/SerializerProperty.h"
#include <glm/gtx/rotate_vector.hpp>

static bool onPlatform(const glm::vec2& position, float yEpsilon, Entity platform);

INSTANCE_IMPL(PlatformerSystem);

PlatformerSystem::PlatformerSystem() : ComponentSystemImpl<PlatformerComponent>(HASH("Platformer", 0x9e52e84a), ComponentType::Complex) {
    PlatformerComponent tc;
    componentSerializer.add(new Property<glm::vec2>(HASH("previous_position", 0x4a11c67f), OFFSET(previousPosition, tc), glm::vec2(0.001, 0)));
    componentSerializer.add(new Property<glm::vec2>(HASH("offset", 0xc4601426), OFFSET(offset, tc), glm::vec2(0.001, 0)));
    componentSerializer.add(new EntityProperty(HASH("on_platform", 0xffbf19ad), OFFSET(onPlatform, tc)));
    componentSerializer.add(new MapProperty<Entity, bool>(HASH("platforms", 0xd6198a31), OFFSET(platforms, tc)));
}

void PlatformerSystem::DoUpdate(float) {
    FOR_EACH_ENTITY_COMPONENT(Platformer, entity, pltf)
        PhysicsComponent* pc = PHYSICS(entity);
        TransformationComponent* tc = TRANSFORM(entity);
        glm::vec2 newPosition(tc->position + glm::rotate(pltf->offset, tc->rotation));

        // if going down
        if (pc->linearVelocity.y < 0) {
            TransformationComponent* tc = TRANSFORM(entity);

            // did we intersect a platform ?
            for (std::map<Entity, bool>::const_iterator it=pltf->platforms.begin(); it != pltf->platforms.end(); ++it) {
                if (!it->second)
                    continue;
                TransformationComponent* pltfTC = TRANSFORM(it->first);
                if (IntersectionUtil::lineLine(
                    pltf->previousPosition, newPosition,
                    pltfTC->position + glm::rotate(glm::vec2(pltfTC->size.x * 0.5, pltfTC->size.y * 0.5), pltfTC->rotation),
                    pltfTC->position + glm::rotate(glm::vec2(-pltfTC->size.x * 0.5, pltfTC->size.y * 0.5), pltfTC->rotation),
                    0)) {
                    // We did intersect...
                    pc->gravity.y = 0;
                    pc->linearVelocity = glm::vec2(0.0f);
                    tc->position.y = pltfTC->position.y + tc->size.y * 0.5;
                    ANIMATION(entity)->name = HASH("jumptorunL2R", 0x9bdaadc5);
                    if (RUNNER(entity)->speed < 0)
                        RENDERING(entity)->flags |= RenderingFlags::MirrorHorizontal;
                    else
                        RENDERING(entity)->flags &= ~(RenderingFlags::MirrorHorizontal);
                    newPosition = tc->position + glm::rotate(pltf->offset, tc->rotation);
                    pltf->onPlatform = it->first;
                    break;
                }
            }
        } else if (pc->linearVelocity.y == 0) {
            if (pltf->onPlatform) {
                if (!onPlatform(newPosition, 0.5, pltf->onPlatform)) {
                    bool foundNew = false;
                    for (std::map<Entity, bool>::const_iterator it=pltf->platforms.begin(); it != pltf->platforms.end(); ++it) {
                        if (it->first == pltf->onPlatform || !it->second)
                            continue;
                        if (onPlatform(newPosition, 0.5, it->first)) {
                            pltf->onPlatform = it->first;
                            foundNew = true;
                        }
                    }
                    if (!foundNew) {
                        pltf->onPlatform = 0;
                        pc->gravity.y= -150;
                        LOGV(1, "No on a platform anymore");
                    }
                } else if (!pltf->platforms[pltf->onPlatform]) {
                    pltf->onPlatform = 0;
                    pc->gravity.y = -150;
                }
            }
        } else {
            pltf->onPlatform = 0;
        }
        pltf->previousPosition = newPosition;
    }
}

static bool onPlatform(const glm::vec2& position, float yEpsilon, Entity platform) {
    const TransformationComponent* pltfTC = TRANSFORM(platform);
    return IntersectionUtil::lineLine(position + glm::vec2(0, yEpsilon), position - glm::vec2(0, yEpsilon),
        pltfTC->position + glm::rotate(glm::vec2(pltfTC->size.x * 0.5, pltfTC->size.y * 0.5), pltfTC->rotation),
        pltfTC->position + glm::rotate(glm::vec2(-pltfTC->size.x * 0.5, pltfTC->size.y * 0.5), pltfTC->rotation),
        0);
}
