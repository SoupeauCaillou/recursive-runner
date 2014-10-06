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
#include "base/StateMachine.h"

#include "base/EntityManager.h"
#include "base/PlacementHelper.h"
#include "systems/TransformationSystem.h"
#include "systems/RenderingSystem.h"
#include "systems/TextSystem.h"
#include "systems/ButtonSystem.h"
#include "systems/AnchorSystem.h"

#include "../RecursiveRunnerGame.h"

class RateScene : public StateHandler<Scene::Enum> {
   RecursiveRunnerGame* game;
   Entity rateText, btnNow, btnLater, btnNever;

   public:
      RateScene(RecursiveRunnerGame* game) : StateHandler<Scene::Enum>("rate") {
         this->game = game;
      }


      void setup(AssetAPI*) override {
#if 0
         Entity entity[4];
         entity[0] = rateText = theEntityManager.CreateEntityFromTemplate("rate/text");
         entity[1] = btnNow = theEntityManager.CreateEntity("rate/button_now",
            EntityType::Persistent, theEntityManager.entityTemplateLibrary.load("rate/button"));
         entity[2] = btnLater = theEntityManager.CreateEntity("rate/button_later",
            EntityType::Persistent, theEntityManager.entityTemplateLibrary.load("rate/button"));
         entity[3] = btnNever = theEntityManager.CreateEntity("rate/button_never",
            EntityType::Persistent, theEntityManager.entityTemplateLibrary.load("rate/button"));

         for (int i = 0; i < 4; i++) {
            ANCHOR(entity[i])->parent = game->cameraEntity;
            ANCHOR(entity[i])->position = glm::vec2(0, -i);
         }
         TEXT(entity[0])->text = "Votez !";
         TEXT(entity[1])->text = "Oui";
         TEXT(entity[2])->text = "Non";
         TEXT(entity[3])->text = "Peut-etre";
#endif
      }

      void onEnter(Scene::Enum) {
         TEXT(rateText)->show =
         TEXT(btnNow)->show =
         BUTTON(btnNow)->enabled =
         TEXT(btnLater)->show =
         BUTTON(btnLater)->enabled =
         TEXT(btnNever)->show =
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
         TEXT(rateText)->show =
         TEXT(btnNow)->show =
         BUTTON(btnNow)->enabled =
         TEXT(btnLater)->show =
         BUTTON(btnLater)->enabled =
         TEXT(btnNever)->show =
         BUTTON(btnNever)->enabled = false;
      }
};

namespace Scene {
    StateHandler<Scene::Enum>* CreateRateSceneHandler(RecursiveRunnerGame* game) {
        return new RateScene(game);
    }
}
