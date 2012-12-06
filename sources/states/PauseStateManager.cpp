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
#include "systems/AnimationSystem.h"
#include "base/TouchInputManager.h"
#include "systems/PhysicsSystem.h"
#include "systems/ButtonSystem.h"

#include "base/TouchInputManager.h"
#include "base/PlacementHelper.h"
#include "../RecursiveRunnerGame.h"

#include "../systems/SessionSystem.h"
#include "../systems/CameraTargetSystem.h"

struct PauseStateManager::PauseStateManagerDatas {
    Entity pauseText;
    Entity continueButton, restartButton, stopButton;
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
    TEXT_RENDERING(pauseText)->text = "PAUSE";
    TEXT_RENDERING(pauseText)->charHeight = 1.;
    TEXT_RENDERING(pauseText)->cameraBitMask = 0x2;
    TEXT_RENDERING(pauseText)->color = Color(13.0 / 255, 5.0/255, 42.0/255);
    TEXT_RENDERING(datas->pauseText)->hide = true;

    Entity buttons[3];
    buttons[0] = datas->continueButton = theEntityManager.CreateEntity();
    buttons[1] = datas->restartButton = theEntityManager.CreateEntity();
    buttons[2] = datas->stopButton = theEntityManager.CreateEntity();
    for (int i = 0; i < 3; ++i) {
        ADD_COMPONENT(buttons[i], Transformation);
        TRANSFORM(buttons[i])->size = Vector2(PlacementHelper::ScreenWidth / 3.1, PlacementHelper::ScreenHeight / 2.);
        TRANSFORM(buttons[i])->parent = game->cameraEntity;
        TRANSFORM(buttons[i])->position = Vector2( (i - 1) * PlacementHelper::ScreenWidth / 3., 0);
        TRANSFORM(buttons[i])->z = 0.95;
        ADD_COMPONENT(buttons[i], Rendering);
        RENDERING(buttons[i])->texture = theRenderingSystem.loadTextureFile("pause");
        RENDERING(buttons[i])->hide = true;
        RENDERING(buttons[i])->cameraBitMask = 0x2;
        RENDERING(buttons[i])->color = Color(119.0 / 255, 119.0 / 255, 119.0 / 255);
        ADD_COMPONENT(buttons[i], Button);
        BUTTON(buttons[i])->enabled = false;
    }
}

void PauseStateManager::earlyEnter() {
}

void PauseStateManager::enter() {
    const SessionComponent* session = SESSION(theSessionSystem.RetrieveAllEntityWithComponent().front());
    // disable physics for runners
    for (unsigned i=0; i<session->runners.size(); i++) {
        PHYSICS(session->runners[i])->mass = 0;
        ANIMATION(session->runners[i])->playbackSpeed = 0;
    }

    //show centered texts
    TRANSFORM(datas->pauseText)->position = theRenderingSystem.cameras[1].worldPosition;
    TEXT_RENDERING(datas->pauseText)->hide = false;

    //show buttons
    RENDERING(datas->continueButton)->hide = false;
    BUTTON(datas->continueButton)->enabled = true;
    RENDERING(datas->restartButton)->hide = false;
    BUTTON(datas->restartButton)->enabled = true;
    RENDERING(datas->stopButton)->hide = false;
    BUTTON(datas->stopButton)->enabled = true;


    //mute music
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

State::Enum PauseStateManager::update(float dt __attribute__((unused))) {
    if (BUTTON(datas->continueButton)->clicked) {
        return State::Game;
    } else if (BUTTON(datas->restartButton)->clicked) {
        // stop music :-[
        RecursiveRunnerGame::endGame();
        RecursiveRunnerGame::startGame(false);
        return State::Game;
    } else if (BUTTON(datas->stopButton)->clicked) {
        return State::Game2Menu;
    }
    return State::Pause;
}

void PauseStateManager::backgroundUpdate(float dt __attribute__((unused))) {

}

void PauseStateManager::exit() {
    const SessionComponent* session = SESSION(theSessionSystem.RetrieveAllEntityWithComponent().front());
    TEXT_RENDERING(datas->pauseText)->hide = true;
    RENDERING(datas->continueButton)->hide = true;
    BUTTON(datas->continueButton)->enabled = false;
    RENDERING(datas->restartButton)->hide = true;
    BUTTON(datas->restartButton)->enabled = false;
    RENDERING(datas->stopButton)->hide = true;
    BUTTON(datas->stopButton)->enabled = false;

    // restore physics for runners
    for (unsigned i=0; i<session->runners.size(); i++) {
        // ^^ this will be done in RunnerSystem PHYSICS(session->runners[i])->mass = 1;
        ANIMATION(session->runners[i])->playbackSpeed = 1.1;
    }
    // update camera position
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
