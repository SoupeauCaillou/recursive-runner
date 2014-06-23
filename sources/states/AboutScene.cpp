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
#include "base/ObjectSerializer.h"
#include "base/EntityManager.h"
#include "base/PlacementHelper.h"
#include "base/TouchInputManager.h"

#include "systems/TransformationSystem.h"
#include "systems/RenderingSystem.h"
#include "systems/ButtonSystem.h"
#include "systems/ADSRSystem.h"
#include "systems/TextSystem.h"
#include "systems/SoundSystem.h"
#include "systems/MusicSystem.h"
#include "systems/PlayerSystem.h"
#include "systems/ParticuleSystem.h"
#include "systems/AutoDestroySystem.h"
#include "systems/AnchorSystem.h"

#include "../systems/SessionSystem.h"
#include "../systems/RunnerSystem.h"

#include "api/LocalizeAPI.h"
#include "api/StorageAPI.h"
#include "util/ScoreStorageProxy.h"

#include "../RecursiveRunnerGame.h"
#include "../Parameters.h"

#include <vector>
#include <mutex>

namespace Button {
    enum Enum {
        Help = 0,
        About,
        Exit,
        Count
    };
}

class AboutScene : public StateHandler<Scene::Enum> {
    RecursiveRunnerGame* game;

    Entity background;
    float alpha, targetAlpha;

    public:
        AboutScene(RecursiveRunnerGame* game) : StateHandler<Scene::Enum>() {
            this->game = game;
        }


        void setup() {
            background  = theEntityManager.CreateEntityFromTemplate("menu/about/bg");
            targetAlpha = RENDERING(background)->color.a;
        }


        ///----------------------------------------------------------------------------//
        ///--------------------- ENTER SECTION ----------------------------------------//
        ///----------------------------------------------------------------------------//
        void onPreEnter(Scene::Enum) {
            alpha = 0;
            RENDERING(background)->show = true;
        }

        bool updatePreEnter(Scene::Enum, float dt) {
            alpha += dt * 2;
            RENDERING(background)->color.a = glm::min(targetAlpha, alpha);
            return (RENDERING(background)->color.a >= targetAlpha);
        }

        void onEnter(Scene::Enum) {
            RENDERING(game->muteBtn)->show = BUTTON(game->muteBtn)->enabled = false;
        }


        Scene::Enum update(float) {
            if (!theTouchInputManager.isTouched(0) && theTouchInputManager.wasTouched(0)) {
                return Scene::Menu;
            }
            return Scene::About;
        }


///----------------------------------------------------------------------------//
///--------------------- EXIT SECTION -----------------------------------------//
///----------------------------------------------------------------------------//
        void onPreExit(Scene::Enum ) {
            alpha = 1;
        }

        bool updatePreExit(Scene::Enum, float dt) {
            alpha -= dt * 2;
            RENDERING(background)->color.a = glm::max(0.0f, alpha);
            return (RENDERING(background)->color.a <= 0.0);
        }

        void onExit(Scene::Enum) {
            RENDERING(background)->show = false;
            RENDERING(game->muteBtn)->show = BUTTON(game->muteBtn)->enabled = true;
        }
};

namespace Scene {
    StateHandler<Scene::Enum>* CreateAboutSceneHandler(RecursiveRunnerGame* game) {
        return new AboutScene(game);
    }
}
