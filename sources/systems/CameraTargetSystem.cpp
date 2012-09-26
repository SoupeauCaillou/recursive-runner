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
 
}

void CameraTargetSystem::DoUpdate(float dt) {
    for(ComponentIt it=components.begin(); it!=components.end(); ++it) {
        Entity a = (*it).first;         
        CameraTargetComponent* ctc = (*it).second;
        
        if (!ctc->enabled)
            continue;

        Vector2 target (TRANSFORM(a)->position + ctc->offset);
        
        Vector2 force = SteeringBehavior::arrive(
            theRenderingSystem.cameraPosition,
            ctc->cameraSpeed,
            target,
            ctc->maxCameraSpeed,
            0.3) * 10;
        // accel = force
        ctc->cameraSpeed += force * dt;
        theRenderingSystem.cameraPosition += ctc->cameraSpeed * dt;
    }
}

