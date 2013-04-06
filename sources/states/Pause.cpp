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
#include "StateManager.h"

#include "base/EntityManager.h"
#include "base/TouchInputManager.h"

#include "systems/TransformationSystem.h"
#include "systems/TextRenderingSystem.h"
#include "systems/MusicSystem.h"
#include "systems/AnimationSystem.h"
#include "systems/PhysicsSystem.h"
#include "systems/ButtonSystem.h"

#include "base/TouchInputManager.h"
#include "base/PlacementHelper.h"
#include "../RecursiveRunnerGame.h"

#include "../systems/SessionSystem.h"
#include "../systems/CameraTargetSystem.h"

struct PauseState::PauseStateDatas {
    Entity title, pauseText;
    Entity continueButton, restartButton, stopButton;
    std::vector<Entity> pausedMusic;
};

PauseState::PauseState(RecursiveRunnerGame* game) : StateManager(State::Pause, game) {
    datas = new PauseStateDatas;
}

PauseState::~PauseState() {
    delete datas;
}

void PauseState::setup() {
    Entity title = datas->title = theEntityManager.CreateEntity("pause_title");
    ADD_COMPONENT(title, Transformation);
    TRANSFORM(title)->size = PlacementHelper::GimpSizeToScreen(theRenderingSystem.getTextureSize("menu-pause"));
    TRANSFORM(title)->z = 0.85;
    TRANSFORM(title)->position = glm::vec2(0, PlacementHelper::ScreenHeight * 0.5 - TRANSFORM(title)->size.y * 0.43);
    //TRANSFORM(title)->rotation = 0.05;
    TRANSFORM(title)->parent = game->cameraEntity;
    ADD_COMPONENT(title, Rendering);
    RENDERING(title)->texture = theRenderingSystem.loadTextureFile("menu-pause");
    RENDERING(title)->show = false;

    Entity pauseText = datas->pauseText = theEntityManager.CreateEntity("pause_text");
    ADD_COMPONENT(pauseText, Transformation);
    TRANSFORM(pauseText)->z = 0.9;
    TRANSFORM(pauseText)->parent = title;
    ADD_COMPONENT(pauseText, TextRendering);
    TEXT_RENDERING(pauseText)->text = "PAUSE";
    TEXT_RENDERING(pauseText)->charHeight = 1.;
    TEXT_RENDERING(datas->pauseText)->show = false;

    std::string textures[] = {"fermer", "recommencer", "reprendre"};
    Entity buttons[3];
    buttons[2] = datas->continueButton = theEntityManager.CreateEntity("pause_button_continue");
    buttons[1] = datas->restartButton = theEntityManager.CreateEntity("pause_button_restart");
    buttons[0] = datas->stopButton = theEntityManager.CreateEntity("pause_button_stop");
    for (int i = 0; i < 3; ++i) {
        ADD_COMPONENT(buttons[i], Transformation);
        TRANSFORM(buttons[i])->size = PlacementHelper::GimpSizeToScreen(theRenderingSystem.getTextureSize(textures[i])) * 1.2f;
        TRANSFORM(buttons[i])->parent = game->cameraEntity;
        int mul = 0;
        if (i == 0) mul = -1;
        else if(i==2) mul=1;
        TRANSFORM(buttons[i])->position = 
            glm::vec2(mul * TRANSFORM(title)->size.x * 0.35,
            0);//PlacementHelper::ScreenHeight * 0.1);
            //- (TRANSFORM(title)->size.Y + TRANSFORM(buttons[i])->size.Y) * 0.45);
        TRANSFORM(buttons[i])->z = 0.9;
        ADD_COMPONENT(buttons[i], Rendering);
        RENDERING(buttons[i])->texture = theRenderingSystem.loadTextureFile(textures[i]);
        RENDERING(buttons[i])->show = false;
        ADD_COMPONENT(buttons[i], Button);
        BUTTON(buttons[i])->enabled = false;
    }
}

void PauseState::willEnter(State::Enum) {
    // TEXT_RENDERING(game->scoreText)->hide = RENDERING(game->scorePanel)->hide = true;
}

void PauseState::enter(State::Enum) {

    const SessionComponent* session = SESSION(theSessionSystem.RetrieveAllEntityWithComponent().front());
    // disable physics for runners
    for (unsigned i=0; i<session->runners.size(); i++) {
        PHYSICS(session->runners[i])->mass = 0;
        ANIMATION(session->runners[i])->playbackSpeed = 0;
    }

    //show text & buttons
    TEXT_RENDERING(datas->pauseText)->show = true;
    RENDERING(datas->continueButton)->show = true;
    BUTTON(datas->continueButton)->enabled = true;
    RENDERING(datas->restartButton)->show = true;
    BUTTON(datas->restartButton)->enabled = true;
    RENDERING(datas->stopButton)->show = true;
    BUTTON(datas->stopButton)->enabled = true;
    // RENDERING(datas->title)->hide = false;
    TRANSFORM(game->scorePanel)->position.x = TRANSFORM(game->cameraEntity)->worldPosition.x;
    TEXT_RENDERING(game->scoreText)->text = "Pause";
    TEXT_RENDERING(game->scoreText)->flags &= ~TextRenderingComponent::IsANumberBit;
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

State::Enum PauseState::update(float) {
    RENDERING(datas->continueButton)->color = BUTTON(datas->continueButton)->mouseOver ? Color("gray") : Color();
    RENDERING(datas->restartButton)->color = BUTTON(datas->restartButton)->mouseOver ? Color("gray") : Color();
    RENDERING(datas->stopButton)->color = BUTTON(datas->stopButton)->mouseOver ? Color("gray") : Color();
 
    if (BUTTON(datas->continueButton)->clicked) {
        return State::Game;
    } else if (BUTTON(datas->restartButton)->clicked) {
        // stop music :-[
        RecursiveRunnerGame::endGame();
        game->setupCamera(CameraMode::Single);
        return State::RestartGame;
    } else if (BUTTON(datas->stopButton)->clicked) {
        RecursiveRunnerGame::endGame();
        return State::Menu;
    }
    return State::Pause;
}

void PauseState::backgroundUpdate(float) {

}

void PauseState::willExit(State::Enum) {
    TEXT_RENDERING(datas->pauseText)->show = false;
    RENDERING(datas->continueButton)->show = false;
    BUTTON(datas->continueButton)->enabled = false;
    RENDERING(datas->restartButton)->show = false;
    BUTTON(datas->restartButton)->enabled = false;
    RENDERING(datas->stopButton)->show = false;
    BUTTON(datas->stopButton)->enabled = false;
    // RENDERING(datas->title)->hide = true;
    // TEXT_RENDERING(game->scoreText)->hide = RENDERING(game->scorePanel)->hide = false;
    TRANSFORM(game->scorePanel)->position.x = 0;
    TEXT_RENDERING(game->scoreText)->flags &= ~TextRenderingComponent::IsANumberBit;
    
}

void PauseState::exit(State::Enum to) {
    std::vector<Entity> sessions = theSessionSystem.RetrieveAllEntityWithComponent();
    if (!sessions.empty()) {
        const SessionComponent* session = SESSION(sessions.front());
        // restore physics for runners
        for (unsigned i=0; i<session->runners.size(); i++) {
            // ^^ this will be done in RunnerSystem PHYSICS(session->runners[i])->mass = 1;
            ANIMATION(session->runners[i])->playbackSpeed = 1.1;
        }
    }
    if (!theMusicSystem.isMuted() && to == State::Game) {
        for (unsigned i=0; i<datas->pausedMusic.size(); i++) {
            MUSIC(datas->pausedMusic[i])->control = MusicControl::Play;
        }
    } else {
        for (unsigned i=0; i<datas->pausedMusic.size(); i++) {
            MUSIC(datas->pausedMusic[i])->fadeOut = 0;
            MUSIC(datas->pausedMusic[i])->control = MusicControl::Stop;
        }
    }
    datas->pausedMusic.clear();
}

bool PauseState::transitionCanExit(State::Enum) {
    return true;
}

bool PauseState::transitionCanEnter(State::Enum) {
    return true;
}
