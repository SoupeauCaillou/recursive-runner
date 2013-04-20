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

#include <vector>

#include "base/EntityManager.h"
#include "base/PlacementHelper.h"

#include <systems/TransformationSystem.h>
#include <systems/ButtonSystem.h>
#include <systems/RenderingSystem.h>

#include "RecursiveRunnerGame.h"
#include <api/CommunicationAPI.h>

class SocialCenterScene : public StateHandler<Scene::Enum> {
    RecursiveRunnerGame* game;
    Entity goToMenuBtn;
    public:

        SocialCenterScene(RecursiveRunnerGame* game) : StateHandler<Scene::Enum>() {
            this->game = game;
        }

        void setup() {
            goToMenuBtn = theEntityManager.CreateEntity("goToMenu_button");
            ADD_COMPONENT(goToMenuBtn, Transformation);
            TRANSFORM(goToMenuBtn)->size = PlacementHelper::GimpSizeToScreen(theRenderingSystem.getTextureSize("reprendre")); //to change
            TRANSFORM(goToMenuBtn)->parent = game->cameraEntity;
            TRANSFORM(goToMenuBtn)->position =
                TRANSFORM(game->cameraEntity)->size * glm::vec2(-0.5, -0.5)
                + glm::vec2(game->buttonSpacing.H, game->buttonSpacing.V);
            TRANSFORM(goToMenuBtn)->z = 0.95;
            ADD_COMPONENT(goToMenuBtn, Rendering);
            RENDERING(goToMenuBtn)->texture = theRenderingSystem.loadTextureFile("reprendre");
            RENDERING(goToMenuBtn)->mirrorH = true;
            ADD_COMPONENT(goToMenuBtn, Button);
            BUTTON(goToMenuBtn)->overSize = 1.2;
        }



        ///----------------------------------------------------------------------------//
        ///--------------------- ENTER SECTION ----------------------------------------//
        ///----------------------------------------------------------------------------//
        void onPreEnter(Scene::Enum) {
            if (game->gameThreadContext->communicationAPI != 0) {
                LOGI("Achievements list:");
                for (auto entry : game->gameThreadContext->communicationAPI->getAllAchievements()) {
                    LOGI("\t" << entry);
                }
                LOGI("Scores list:");
                for (auto entry : game->gameThreadContext->communicationAPI->getScores(0, CommunicationAPI::Score::ALL, 1, 10)) {
                    LOGI("\t" << entry);
                }
            }
            RENDERING(goToMenuBtn)->show = true;
        }

        void onEnter(Scene::Enum) {
            BUTTON(goToMenuBtn)->enabled = true;
        }


        ///----------------------------------------------------------------------------//
        ///--------------------- UPDATE SECTION ---------------------------------------//
        ///----------------------------------------------------------------------------//

        Scene::Enum update(float) {
            if (BUTTON(goToMenuBtn)->clicked)
                return Scene::Menu;
            return Scene::SocialCenter;
        }


        ///----------------------------------------------------------------------------//
        ///--------------------- EXIT SECTION -----------------------------------------//
        ///----------------------------------------------------------------------------//
        void onPreExit(Scene::Enum) {
            BUTTON(goToMenuBtn)->enabled = false;
        }


        void onExit(Scene::Enum) {
            RENDERING(goToMenuBtn)->show = false;
        }
};

namespace Scene {
    StateHandler<Scene::Enum>* CreateSocialCenterSceneHandler(RecursiveRunnerGame* game) {
        return new SocialCenterScene(game);
    }
}
