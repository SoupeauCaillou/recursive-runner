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
#pragma once

#include "base/Color.h"
#include "systems/System.h"

class Color;

struct Statistics {
    int score;
    struct Runner {
        int coinsCollected;
        float lifetime;
        int pointScored;
        int killed;
        int maxOldness;
        int maxBonus;
    } runner[10];
    Color color[10];
};

struct Platform {
    Platform() {
        active = false;
        switches[0].owner = switches[1].owner = 0;
        switches[0].state = switches[1].state = false;
    }
    struct {
        Entity entity, owner;
        bool state;
    } switches[2];
    Entity platform;
    bool active;
};

struct SessionComponent {
    SessionComponent() : numPlayers(1), currentRunner(0), userInputEnabled(true) {memset(&stats, 0 ,sizeof(Statistics));}
    unsigned numPlayers;
    Entity currentRunner;
    bool userInputEnabled;
    std::vector<Entity> runners, coins, players, links, sparkling, gains;
    std::vector<Platform> platforms;
    Statistics stats;
};

#define theSessionSystem SessionSystem::GetInstance()
#define SESSION(e) theSessionSystem.Get(e)

UPDATABLE_SYSTEM(Session)
};
