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
#include "systems/TextRenderingSystem.h"
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
    Entity title, pauseText;
    Entity continueButton, restartButton, stopButton;
    std::vector<Entity> pausedMusic;

    public:

    PauseScene(RecursiveRunnerGame* game) : StateHandler<Scene::Enum>() {
        this->game = game;
    }

    void setup() {
        title = theEntityManager.CreateEntity("pause_title");
        ADD_COMPONENT(title, Transformation);
        TRANSFORM(title)->size = PlacementHelper::GimpSizeToScreen(theRenderingSystem.getTextureSize("menu-pause"));
        ADD_COMPONENT(title, Anchor);
        ANCHOR(title)->z = 0.85;
        ANCHOR(title)->position = glm::vec2(0, PlacementHelper::ScreenHeight * 0.5 - TRANSFORM(title)->size.y * 0.43);
        ANCHOR(title)->parent = game->cameraEntity;
        ADD_COMPONENT(title, Rendering);
        RENDERING(title)->texture = theRenderingSystem.loadTextureFile("menu-pause");
        RENDERING(title)->show = false;

        pauseText = theEntityManager.CreateEntity("pause_text");
        ADD_COMPONENT(pauseText, Transformation);
        ADD_COMPONENT(pauseText, Anchor);
        ANCHOR(pauseText)->z = 0.9;
        ANCHOR(pauseText)->parent = title;
        ADD_COMPONENT(pauseText, TextRendering);
        TEXT_RENDERING(pauseText)->text = "PAUSE";
        TEXT_RENDERING(pauseText)->charHeight = 1.;
        TEXT_RENDERING(pauseText)->show = false;

        std::string textures[] = {"fermer", "recommencer", "reprendre"};
        Entity buttons[3];
        buttons[2] = continueButton = theEntityManager.CreateEntity("pause_button_continue");
        buttons[1] = restartButton = theEntityManager.CreateEntity("pause_button_restart");
        buttons[0] = stopButton = theEntityManager.CreateEntity("pause_button_stop");
        for (int i = 0; i < 3; ++i) {
            ADD_COMPONENT(buttons[i], Transformation);
            TRANSFORM(buttons[i])->size = PlacementHelper::GimpSizeToScreen(theRenderingSystem.getTextureSize(textures[i])) * 1.2f;
            ADD_COMPONENT(buttons[i], Anchor);
            ANCHOR(buttons[i])->parent = game->cameraEntity;
            int mul = 0;
            if (i == 0) mul = -1;
            else if(i==2) mul=1;
            ANCHOR(buttons[i])->position =
                glm::vec2(mul * TRANSFORM(title)->size.x * 0.35,
                0);//PlacementHelper::ScreenHeight * 0.1);
                //- (TRANSFORM(title)->size.Y + TRANSFORM(buttons[i])->size.Y) * 0.45);
            ANCHOR(buttons[i])->z = 0.9;
            ADD_COMPONENT(buttons[i], Rendering);
            RENDERING(buttons[i])->texture = theRenderingSystem.loadTextureFile(textures[i]);
            RENDERING(buttons[i])->show = false;
            ADD_COMPONENT(buttons[i], Button);
            BUTTON(buttons[i])->enabled = false;
        }
    }


    void onEnter(Scene::Enum) {
        const SessionComponent* session = SESSION(theSessionSystem.RetrieveAllEntityWithComponent().front());
        // disable physics for runners
        for (unsigned i=0; i<session->runners.size(); i++) {
            PHYSICS(session->runners[i])->mass = 0;
            ANIMATION(session->runners[i])->playbackSpeed = 0;
        }

        //show text & buttons
        TEXT_RENDERING(pauseText)->show = true;
        RENDERING(continueButton)->show = true;
        BUTTON(continueButton)->enabled = true;
        RENDERING(restartButton)->show = true;
        BUTTON(restartButton)->enabled = true;
        RENDERING(stopButton)->show = true;
        BUTTON(stopButton)->enabled = true;
        // RENDERING(title)->hide = false;
        TRANSFORM(game->scorePanel)->position.x = TRANSFORM(game->cameraEntity)->position.x;
        TEXT_RENDERING(game->scoreText)->text = "Pause";
        TEXT_RENDERING(game->scoreText)->flags &= ~TextRenderingComponent::IsANumberBit;
        //mute music
        if (!theMusicSystem.isMuted()) {
            std::vector<Entity> musics = theMusicSystem.RetrieveAllEntityWithComponent();
            for (unsigned i=0; i<musics.size(); i++) {
                if (MUSIC(musics[i])->control == MusicControl::Play) {
                    MUSIC(musics[i])->control = MusicControl::Pause;
                    pausedMusic.push_back(musics[i]);
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
        TEXT_RENDERING(pauseText)->show = false;
        RENDERING(continueButton)->show = false;
        BUTTON(continueButton)->enabled = false;
        RENDERING(restartButton)->show = false;
        BUTTON(restartButton)->enabled = false;
        RENDERING(stopButton)->show = false;
        BUTTON(stopButton)->enabled = false;
        // RENDERING(title)->hide = true;
        // TEXT_RENDERING(game->scoreText)->hide = RENDERING(game->scorePanel)->hide = false;
        TRANSFORM(game->scorePanel)->position.x = 0;
        TEXT_RENDERING(game->scoreText)->flags &= ~TextRenderingComponent::IsANumberBit;
    }

    void onExit(Scene::Enum to) {
        std::vector<Entity> sessions = theSessionSystem.RetrieveAllEntityWithComponent();
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
