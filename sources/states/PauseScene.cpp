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

#include "systems/ADSRSystem.h"
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

#include "base/SceneState.h"


class PauseScene : public SceneState<Scene::Enum> {
    RecursiveRunnerGame* game;

    std::vector<Entity> pausedMusic;
    float panelPosition, panelAccum;

    public:

    PauseScene(RecursiveRunnerGame* game) : SceneState<Scene::Enum>("pause", SceneEntityMode::Fading, SceneEntityMode::Fading) {
        this->game = game;
    }

    void onPreEnter(Scene::Enum f) override {
        SceneState<Scene::Enum>::onPreEnter(f);

        // move up score panel
        ADSR(game->scorePanel)->active = false;
        panelPosition = TRANSFORM(game->scorePanel)->position.x;
    }

    bool updatePreEnter(Scene::Enum f, float dt) override {
        bool b = SceneState<Scene::Enum>::updatePreEnter(f, dt);

        return b && ADSR(game->scorePanel)->value <= ADSR(game->scorePanel)->idleValue;
    }

    void onEnter(Scene::Enum f) {
        SceneState<Scene::Enum>::onEnter(f);

        const SessionComponent* session = SESSION(theSessionSystem.RetrieveAllEntityWithComponent().front());
        // disable physics for runners
        for (unsigned i=0; i<session->runners.size(); i++) {
            PHYSICS(session->runners[i])->mass = 0;
            ANIMATION(session->runners[i])->playbackSpeed = 0;
        }

        TRANSFORM(game->scorePanel)->position.x = TRANSFORM(game->cameraEntity)->position.x;
        TEXT(game->scoreText)->text = "Pause";
        TEXT(game->scoreText)->flags &= ~TextComponent::IsANumberBit;
        ADSR(game->scorePanel)->active = true;

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
        RENDERING(entities[HASH("pause/reprendre", 0x6f650d56)])->color = BUTTON(entities[HASH("pause/reprendre", 0x6f650d56)])->mouseOver ? Color("gray") : Color();
        RENDERING(entities[HASH("pause/recommencer", 0x1829bb20)])->color = BUTTON(entities[HASH("pause/recommencer", 0x1829bb20)])->mouseOver ? Color("gray") : Color();
        RENDERING(entities[HASH("pause/fermer", 0x1375e49c)])->color = BUTTON(entities[HASH("pause/fermer", 0x1375e49c)])->mouseOver ? Color("gray") : Color();

        if (BUTTON(entities[HASH("pause/reprendre", 0x6f650d56)])->clicked) {
            return Scene::Game;
        } else if (BUTTON(entities[HASH("pause/recommencer", 0x1829bb20)])->clicked) {
            // stop music :-[
            RecursiveRunnerGame::endGame();
            game->setupCamera(CameraMode::Single);
            return Scene::RestartGame;
        } else if (BUTTON(entities[HASH("pause/fermer", 0x1375e49c)])->clicked) {
            RecursiveRunnerGame::endGame();
            return Scene::Menu;
        }
        return Scene::Pause;
    }

    void onPreExit(Scene::Enum f) {
        SceneState<Scene::Enum>::onPreExit(f);

        ADSR(game->scorePanel)->active = false;
    }

    bool updatePreExit(Scene::Enum f, float dt) override {
        bool b = SceneState<Scene::Enum>::updatePreExit(f, dt);
        return b && ADSR(game->scorePanel)->value <= ADSR(game->scorePanel)->idleValue;
    }

    void onExit(Scene::Enum to) {
        SceneState<Scene::Enum>::onExit(to);
        TRANSFORM(game->scorePanel)->position.x = 0;
        TEXT(game->scoreText)->text = "";
        TEXT(game->scoreText)->flags &= ~TextComponent::IsANumberBit;
        ADSR(game->scorePanel)->active = true;
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
