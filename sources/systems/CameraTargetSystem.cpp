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
#include "CameraTargetSystem.h"
#include "base/MathUtil.h"
#include "systems/TransformationSystem.h"
#include "systems/RenderingSystem.h"
#include "util/IntersectionUtil.h"
#include "steering/SteeringBehavior.h"

INSTANCE_IMPL(CameraTargetSystem);

CameraTargetSystem::CameraTargetSystem() : ComponentSystemImpl<CameraTargetComponent>("CameraTarget") {
    CameraTargetComponent tc;
    componentSerializer.add(new Property<int>(OFFSET(cameraIndex, tc)));
    componentSerializer.add(new Property<Vector2>(OFFSET(offset, tc), Vector2(0.001, 0)));
    componentSerializer.add(new Property<float>(OFFSET(maxCameraSpeed, tc), 0.001));
    componentSerializer.add(new Property<bool>(OFFSET(enabled, tc)));
    componentSerializer.add(new Property<Vector2>(OFFSET(cameraSpeed.X, tc), Vector2(0.001, 0)));
}

void CameraTargetSystem::DoUpdate(float dt) {
    FOR_EACH_ENTITY_COMPONENT(CameraTarget, a, ctc)
        if (!ctc->enabled)
            continue;
        Vector2 target (TRANSFORM(a)->position + ctc->offset);
        Vector2 force = SteeringBehavior::arrive(
            theRenderingSystem.cameras[ctc->cameraIndex].worldPosition,
            ctc->cameraSpeed,
            target,
            ctc->maxCameraSpeed,
            0.3) * 10;
        // accel = force
        ctc->cameraSpeed += force * dt;
        theRenderingSystem.cameras[ctc->cameraIndex].worldPosition.X += ctc->cameraSpeed.X * dt;
    }
}

