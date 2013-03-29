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
#pragma once

#include "systems/System.h"
#include "base/Color.h"
#include <glm/glm.hpp>

struct CollisionZone {
    CollisionZone(float x=0,float y=0,float w=0, float h=0, float r=0) {
        size.x = w / 200.0; size.y = h / 210.0;
        position.x = x / 200.0 - 0.5;// + size.X * 0.5;
        position.y = 0.5 - y / 210.0;// - size.Y * 0.5;
        rotation = r;
    }
    glm::vec2 position, size;
    float rotation;
};

struct RunnerComponent {
    RunnerComponent() : finished(false), ghost(false), killed(false), startTime(0), elapsed(0), jumpingSince(0), currentJump(0), oldNessBonus(0), coinSequenceBonus(1) { }
    Entity playerOwner, collisionZone;
    glm::vec2 startPoint, endPoint;
    Color color;
    float speed;
    bool finished, ghost, killed;
    float startTime, elapsed, jumpingSince;
    int currentJump, oldNessBonus, coinSequenceBonus;
    std::vector<float> jumpTimes;
    std::vector<float> jumpDurations;
    std::vector<Entity> coins;
};

#define theRunnerSystem RunnerSystem::GetInstance()
#define RUNNER(e) theRunnerSystem.Get(e)

UPDATABLE_SYSTEM(Runner)
public:
    static float MinJumpDuration;
    static float MaxJumpDuration;
};
