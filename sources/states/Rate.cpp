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
#include "base/PlacementHelper.h"
#include "systems/TransformationSystem.h"
#include "systems/RenderingSystem.h"
#include "systems/TextRenderingSystem.h"
#include "systems/ButtonSystem.h"
#include "systems/AnchorSystem.h"

#include "../RecursiveRunnerGame.h"

class RateScene : public StateHandler<Scene::Enum> {
   RecursiveRunnerGame* game;
   Entity rateText, btnNow, btnLater, btnNever;

   public:
      RateScene(RecursiveRunnerGame* game) : StateHandler<Scene::Enum>() {
         this->game = game;
      }


      void setup() {
         Entity entity[4];
         entity[0] = rateText = theEntityManager.CreateEntity("rate_text");
         entity[1] = btnNow = theEntityManager.CreateEntity("rate_button_now");
         entity[2] = btnLater = theEntityManager.CreateEntity("rate_button_later");
         entity[3] = btnNever = theEntityManager.CreateEntity("rate_button_never");

         for (int i = 0; i < 4; i++) {
            ADD_COMPONENT(entity[i], Transformation);
            ADD_COMPONENT(entity[i], Anchor);
            ANCHOR(entity[i])->z = 0.9;
            ANCHOR(entity[i])->parent = game->cameraEntity;
            ANCHOR(entity[i])->position = glm::vec2(0, -i);

            ADD_COMPONENT(entity[i], TextRendering);
            TEXT_RENDERING(entity[i])->charHeight = 1.;
            TEXT_RENDERING(entity[i])->color = Color(13.0 / 255, 5.0/255, 42.0/255);
            TEXT_RENDERING(entity[i])->show = false;

            if (i)
               ADD_COMPONENT(entity[i], Button);
         }
         TEXT_RENDERING(entity[0])->text = "Votez !";
         TEXT_RENDERING(entity[1])->text = "Oui";
         TEXT_RENDERING(entity[2])->text = "Non";
         TEXT_RENDERING(entity[3])->text = "Peut-etre";

      }

      void onEnter(Scene::Enum) {
         TEXT_RENDERING(rateText)->show = true;
         TEXT_RENDERING(btnNow)->show = true;
         BUTTON(btnNow)->enabled = true;
         TEXT_RENDERING(btnLater)->show = true;
         BUTTON(btnLater)->enabled = true;
         TEXT_RENDERING(btnNever)->show = true;
         BUTTON(btnNever)->enabled = true;
      }

      Scene::Enum update(float) {
         if (BUTTON(btnNow)->clicked) {
            game->gameThreadContext->communicationAPI->rateItNow();
            return Scene::Menu;
         } else if (BUTTON(btnNever)->clicked) {
            game->gameThreadContext->communicationAPI->rateItNever();
            return Scene::Menu;
         } else if (BUTTON(btnLater)->clicked) {
            game->gameThreadContext->communicationAPI->rateItLater();
            return Scene::Menu;
         }

         return Scene::Rate;
      }

      void onPreExit(Scene::Enum) {
         TEXT_RENDERING(rateText)->show = false;
         TEXT_RENDERING(btnNow)->show = false;
         BUTTON(btnNow)->enabled = false;
         TEXT_RENDERING(btnLater)->show = false;
         BUTTON(btnLater)->enabled = false;
         TEXT_RENDERING(btnNever)->show = false;
         BUTTON(btnNever)->enabled = false;
      }
};

namespace Scene {
    StateHandler<Scene::Enum>* CreateRateSceneHandler(RecursiveRunnerGame* game) {
        return new RateScene(game);
    }
}
