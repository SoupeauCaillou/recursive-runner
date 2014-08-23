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
#include "base/StateMachine.h"
#include "base/EntityManager.h"
#include "base/TouchInputManager.h"

#include "systems/TransformationSystem.h"
#include "systems/TextSystem.h"
#include "systems/MusicSystem.h"
#include "systems/AnimationSystem.h"
#include "systems/PhysicsSystem.h"
#include "systems/ButtonSystem.h"
#include "systems/AnchorSystem.h"

#include "base/TouchInputManager.h"
#include "base/PlacementHelper.h"
#include "../RecursiveRunnerGame.h"

#include "../systems/SessionSystem.h"
#include "../systems/CameraTargetSystem.h"


class PauseScene : public StateHandler<Scene::Enum> {
    RecursiveRunnerGame* game;

    Entity continueButton, restartButton, stopButton;
    std::vector<Entity> pausedMusic;

    public:

    PauseScene(RecursiveRunnerGame* game) : StateHandler<Scene::Enum>() {
        this->game = game;
    }

    void setup() {
        stopButton = theEntityManager.CreateEntityFromTemplate("pause/fermer");
        restartButton = theEntityManager.CreateEntityFromTemplate("pause/recommencer");
        continueButton = theEntityManager.CreateEntityFromTemplate("pause/reprendre");
    }


    void onEnter(Scene::Enum) {
        const SessionComponent* session = SESSION(theSessionSystem.RetrieveAllEntityWithComponent().front());
        // disable physics for runners
        for (unsigned i=0; i<session->runners.size(); i++) {
            PHYSICS(session->runners[i])->mass = 0;
            ANIMATION(session->runners[i])->playbackSpeed = 0;
        }

        //show text & buttons
        RENDERING(continueButton)->show = true;
        BUTTON(continueButton)->enabled = true;
        RENDERING(restartButton)->show = true;
        BUTTON(restartButton)->enabled = true;
        RENDERING(stopButton)->show = true;
        BUTTON(stopButton)->enabled = true;

        TRANSFORM(game->scorePanel)->position.x = TRANSFORM(game->cameraEntity)->position.x;
        TEXT(game->scoreText)->text = "Pause";
        TEXT(game->scoreText)->flags &= ~TextComponent::IsANumberBit;
        //mute music
        if (!theMusicSystem.isMuted()) {
            const auto& musics = theMusicSystem.RetrieveAllEntityWithComponent();
            for (auto music: musics) {
                if (MUSIC(music)->control == MusicControl::Play) {
                    MUSIC(music)->control = MusicControl::Pause;
                    pausedMusic.push_back(music);
                }
            }
        }
    }

    Scene::Enum update(float) {
        RENDERING(continueButton)->color = BUTTON(continueButton)->mouseOver ? Color("gray") : Color();
        RENDERING(restartButton)->color = BUTTON(restartButton)->mouseOver ? Color("gray") : Color();
        RENDERING(stopButton)->color = BUTTON(stopButton)->mouseOver ? Color("gray") : Color();

        if (BUTTON(continueButton)->clicked) {
            return Scene::Game;
        } else if (BUTTON(restartButton)->clicked) {
            // stop music :-[
            RecursiveRunnerGame::endGame();
            game->setupCamera(CameraMode::Single);
            return Scene::RestartGame;
        } else if (BUTTON(stopButton)->clicked) {
            RecursiveRunnerGame::endGame();
            return Scene::Menu;
        }
        return Scene::Pause;
    }

    void onPreExit(Scene::Enum) {
        RENDERING(continueButton)->show = false;
        BUTTON(continueButton)->enabled = false;
        RENDERING(restartButton)->show = false;
        BUTTON(restartButton)->enabled = false;
        RENDERING(stopButton)->show = false;
        BUTTON(stopButton)->enabled = false;

        // TEXT(game->scoreText)->hide = RENDERING(game->scorePanel)->hide = false;
        TRANSFORM(game->scorePanel)->position.x = 0;
        TEXT(game->scoreText)->flags &= ~TextComponent::IsANumberBit;
    }

    void onExit(Scene::Enum to) {
        const auto& sessions = theSessionSystem.RetrieveAllEntityWithComponent();
        if (!sessions.empty()) {
            const SessionComponent* session = SESSION(sessions.front());
            // restore physics for runners
            for (unsigned i=0; i<session->runners.size(); i++) {
                // ^^ this will be done in RunnerSystem PHYSICS(session->runners[i])->mass = 1;
                ANIMATION(session->runners[i])->playbackSpeed = 1.1;
            }
        }
        if (!theMusicSystem.isMuted() && to == Scene::Game) {
            for (unsigned i=0; i<pausedMusic.size(); i++) {
                MUSIC(pausedMusic[i])->control = MusicControl::Play;
            }
        } else {
            for (unsigned i=0; i<pausedMusic.size(); i++) {
                MUSIC(pausedMusic[i])->fadeOut = 0;
                MUSIC(pausedMusic[i])->control = MusicControl::Stop;
            }
        }
        pausedMusic.clear();
    }
};

namespace Scene {
    StateHandler<Scene::Enum>* CreatePauseSceneHandler(RecursiveRunnerGame* game) {
        return new PauseScene(game);
    }
}
