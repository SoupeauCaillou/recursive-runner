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
#include "CameraTargetSystem.h"
#include "systems/TransformationSystem.h"
#include "systems/RenderingSystem.h"
#include "RunnerSystem.h"
#include "util/IntersectionUtil.h"
#include "util/SerializerProperty.h"
#include "steering/SteeringBehavior.h"
#include "base/PlacementHelper.h"
#include "../Parameters.h"

INSTANCE_IMPL(CameraTargetSystem);

CameraTargetSystem::CameraTargetSystem() : ComponentSystemImpl<CameraTargetComponent>(HASH("CameraTarget", 0xf279bb66)) {
    CameraTargetComponent tc;
    componentSerializer.add(new Property<Entity>(HASH("camera", 0x526b9e0c), OFFSET(camera, tc)));
    componentSerializer.add(new Property<glm::vec2>(HASH("offset", 0xc4601426), OFFSET(offset, tc), glm::vec2(0.001, 0)));
    componentSerializer.add(new Property<float>(HASH("max_camera_speed", 0xec97a9bf), OFFSET(maxCameraSpeed, tc), 0.001));
    componentSerializer.add(new Property<bool>(HASH("enabled", 0x1d6995b7), OFFSET(enabled, tc)));
    componentSerializer.add(new Property<glm::vec2>(HASH("camera_speed", 0x7c76c3a2), OFFSET(cameraSpeed, tc), glm::vec2(0.001, 0)));
}

static glm::vec2 arrive(const glm::vec2& pos, const glm::vec2& ,const glm::vec2& targetPos, float maxSpeed, float deceleration) {
    glm::vec2 toTarget (targetPos - pos);
    float d = glm::length(toTarget);

    if (d > 0) {
        toTarget = glm::normalize(toTarget);
        float speed = glm::min(d / deceleration, maxSpeed);
        glm::vec2 desiredVelocity(toTarget * speed);
        return desiredVelocity;
    }
    return glm::vec2(0.0f, 0.0f);
}


void CameraTargetSystem::DoUpdate(float dt) {
    FOR_EACH_ENTITY_COMPONENT(CameraTarget, a, ctc)
        if (!ctc->enabled)
            continue;
        glm::vec2 target (TRANSFORM(a)->position + ctc->offset);

        // limit offset to valid position
        if (target.x < - PlacementHelper::ScreenSize.x * (param::LevelSize * 0.5 - 0.5)) {
            target.x = - PlacementHelper::ScreenSize.x * (param::LevelSize * 0.5 - 0.5);
        } else if (target.x > PlacementHelper::ScreenSize.x * (param::LevelSize * 0.5 - 0.5)) {
            target.x = PlacementHelper::ScreenSize.x * (param::LevelSize * 0.5 - 0.5);
        }

        glm::vec2 force = arrive(
            TRANSFORM(ctc->camera)->position,
            ctc->cameraSpeed,
            target,
            ctc->maxCameraSpeed,
            0.3);
        // accel = force
        ctc->cameraSpeed = force; //+= force;

        // and camera must move in the same direction as the target
        if (ctc->cameraSpeed.x * RUNNER(a)->speed < 0) {
            ctc->cameraSpeed = glm::vec2(0.0f);
        } else {
            TRANSFORM(ctc->camera)->position.x += ctc->cameraSpeed.x * dt;
        }
    }
}
