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
#pragma once

#include "systems/System.h"
#include "base/Vector2.h"

struct RunnerComponent {
    RunnerComponent() : finished(false), startTime(0), currentJump(0) {}
    Vector2 startPoint, endPoint;
    float maxSpeed;
    float speed;
    bool finished;
    float startTime, elapsed;
    int currentJump;
    std::vector<float> jumpTimes; 
    std::vector<Entity> coins;
};

#define theRunnerSystem RunnerSystem::GetInstance()
#define RUNNER(e) theRunnerSystem.Get(e)

UPDATABLE_SYSTEM(Runner)
};
