/*
 This file is part of Recursive Runner.

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
#include "StateManager.h"

#include "systems/TransformationSystem.h"
#include "systems/TextRenderingSystem.h"
#include "systems/MusicSystem.h"
#include "base/TouchInputManager.h"
#include "systems/PhysicsSystem.h"
#include "../RecursiveRunnerGame.h"

struct PauseStateManager::PauseStateManagerDatas {
    Entity pauseText;
    std::vector<Entity> pausedMusic;
};

PauseStateManager::PauseStateManager(RecursiveRunnerGame* game) : StateManager(State::Pause, game) {
    datas = new PauseStateManagerDatas;
}

PauseStateManager::~PauseStateManager() {
    delete datas;
}

void PauseStateManager::setup() {
    Entity pauseText = datas->pauseText = theEntityManager.CreateEntity();
    ADD_COMPONENT(pauseText, Transformation);
    TRANSFORM(pauseText)->z = 0.9;
    TRANSFORM(pauseText)->rotation = 0.0;
    ADD_COMPONENT(pauseText, TextRendering);
    TEXT_RENDERING(pauseText)->text = "PAUSED (touch to continue)";
    TEXT_RENDERING(pauseText)->charHeight = 1.;
    TEXT_RENDERING(pauseText)->cameraBitMask = 0x2;
    TEXT_RENDERING(pauseText)->color = Color(13.0 / 255, 5.0/255, 42.0/255);
    TEXT_RENDERING(datas->pauseText)->hide = true;
}

void PauseStateManager::earlyEnter() {
}

void PauseStateManager::enter() {
    // disable physics for runners
    for (unsigned i=0; i<game->gameTempVars.runners[0].size(); i++) {
        PHYSICS(game->gameTempVars.runners[0][i])->mass = 0;
    }
    TRANSFORM(datas->pauseText)->position = theRenderingSystem.cameras[1].worldPosition;
    TEXT_RENDERING(datas->pauseText)->hide = false;
    if (!theMusicSystem.isMuted()) {
        std::vector<Entity> musics = theMusicSystem.RetrieveAllEntityWithComponent();
        for (unsigned i=0; i<musics.size(); i++) {
            if (MUSIC(musics[i])->control == MusicControl::Play) {
                MUSIC(musics[i])->control = MusicControl::Pause;
                datas->pausedMusic.push_back(musics[i]);
            }
        }
    }
}

State::Enum PauseStateManager::update(float dt) {
    if (!theTouchInputManager.isTouched(0) && theTouchInputManager.wasTouched(0)) {
        return State::Game;
    }
    return State::Pause;
}

void PauseStateManager::backgroundUpdate(float dt __attribute__((unused))) {

}

void PauseStateManager::exit() {
    TEXT_RENDERING(datas->pauseText)->hide = true;
    // restore physics for runners
    for (unsigned i=0; i<game->gameTempVars.runners[0].size(); i++) {
        PHYSICS(game->gameTempVars.runners[0][i])->mass = 1;
    }
    if (!theMusicSystem.isMuted()) {
        for (unsigned i=0; i<datas->pausedMusic.size(); i++) {
            MUSIC(datas->pausedMusic[i])->control = MusicControl::Play;
        }
    }
    datas->pausedMusic.clear();
}

void PauseStateManager::lateExit() {

}

bool PauseStateManager::transitionCanExit() {
    return true;
}

bool PauseStateManager::transitionCanEnter() {
    return true;
}