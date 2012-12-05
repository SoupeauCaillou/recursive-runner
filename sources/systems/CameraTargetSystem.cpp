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
#include "CameraTargetSystem.h"
#include "base/MathUtil.h"
#include "systems/TransformationSystem.h"
#include "systems/RenderingSystem.h"
#include "util/IntersectionUtil.h"
#include "steering/SteeringBehavior.h"

INSTANCE_IMPL(CameraTargetSystem);
 
CameraTargetSystem::CameraTargetSystem() : ComponentSystemImpl<CameraTargetComponent>("CameraTarget") {
    CameraTargetComponent tc;
    componentSerializer.add(new Property(OFFSET(cameraIndex, tc), sizeof(int)));
    componentSerializer.add(new EpsilonProperty<float>(OFFSET(offset.X, tc), 0.001));
    componentSerializer.add(new EpsilonProperty<float>(OFFSET(offset.Y, tc), 0.001));
    componentSerializer.add(new EpsilonProperty<float>(OFFSET(maxCameraSpeed, tc), 0.001));
    componentSerializer.add(new Property(OFFSET(enabled, tc), sizeof(bool)));
    componentSerializer.add(new EpsilonProperty<float>(OFFSET(cameraSpeed.X, tc), 0.001));
    componentSerializer.add(new EpsilonProperty<float>(OFFSET(cameraSpeed.Y, tc), 0.001));
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

