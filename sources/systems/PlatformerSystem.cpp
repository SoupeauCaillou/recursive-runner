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
#include "PlatformerSystem.h"
#include "base/MathUtil.h"
#include "systems/TransformationSystem.h"
#include "systems/PhysicsSystem.h"
#include "systems/AnimationSystem.h"
#include "systems/RenderingSystem.h"
#include "systems/RunnerSystem.h"
#include "util/IntersectionUtil.h"

static bool onPlatform(const Vector2& position, float yEpsilon, Entity platform);

INSTANCE_IMPL(PlatformerSystem);

PlatformerSystem::PlatformerSystem() : ComponentSystemImpl<PlatformerComponent>("Platformer") {
    PlatformerComponent tc;
    componentSerializer.add(new Property(OFFSET(previousPosition, tc), sizeof(Vector2)));
    componentSerializer.add(new VectorProperty<Entity>(OFFSET(platforms, tc)));
}

void PlatformerSystem::DoUpdate(float) {
    FOR_EACH_ENTITY_COMPONENT(Platformer, entity, pltf)
        PhysicsComponent* pc = PHYSICS(entity);
        TransformationComponent* tc = TRANSFORM(entity);
        Vector2 newPosition(tc->worldPosition + Vector2::Rotate(pltf->offset, tc->worldRotation));

        // if going down
        if (pc->linearVelocity.Y < 0) {
            TransformationComponent* tc = TRANSFORM(entity);

            // did we intersect a platform ?
            for (std::map<Entity, bool>::const_iterator it=pltf->platforms.begin(); it != pltf->platforms.end(); ++it) {
                if (!it->second)
                    continue;
                TransformationComponent* pltfTC = TRANSFORM(it->first);
                if (IntersectionUtil::lineLine(
                    pltf->previousPosition, newPosition,
                    pltfTC->worldPosition + Vector2::Rotate(Vector2(pltfTC->size.X * 0.5, pltfTC->size.Y * 0.5), pltfTC->worldRotation),
                    pltfTC->worldPosition + Vector2::Rotate(Vector2(-pltfTC->size.X * 0.5, pltfTC->size.Y * 0.5), pltfTC->worldRotation),
                    0)) {
                    // We did intersect...
                    pc->gravity.Y = 0;
                    pc->linearVelocity = Vector2::Zero;
                    tc->position.Y = pltfTC->worldPosition.Y + tc->size.Y * 0.5;
                    ANIMATION(entity)->name = "jumptorunL2R";
                    RENDERING(entity)->mirrorH = (RUNNER(entity)->speed < 0);
                    newPosition = tc->position + Vector2::Rotate(pltf->offset, tc->worldRotation);
                    pltf->onPlatform = it->first;
                    break;
                }
            }
        } else if (pc->linearVelocity.Y == 0) {
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
                        pc->gravity.Y = -150;
                        std::cout << "No on a platform anymore" << std::endl;
                    }
                } else if (!pltf->platforms[pltf->onPlatform]) {
                    pltf->onPlatform = 0;
                    pc->gravity.Y = -150;
                }
            }
        } else {
            pltf->onPlatform = 0;
        }
        pltf->previousPosition = newPosition;
    }
}

static bool onPlatform(const Vector2& position, float yEpsilon, Entity platform) {
    const TransformationComponent* pltfTC = TRANSFORM(platform);
    return IntersectionUtil::lineLine(position + Vector2(0, yEpsilon), position - Vector2(0, yEpsilon),
        pltfTC->worldPosition + Vector2::Rotate(Vector2(pltfTC->size.X * 0.5, pltfTC->size.Y * 0.5), pltfTC->worldRotation),
        pltfTC->worldPosition + Vector2::Rotate(Vector2(-pltfTC->size.X * 0.5, pltfTC->size.Y * 0.5), pltfTC->worldRotation),
        0);
}
