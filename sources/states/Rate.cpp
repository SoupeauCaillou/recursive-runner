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
#include "base/PlacementHelper.h"
#include "systems/TransformationSystem.h"
#include "systems/RenderingSystem.h"
#include "systems/TextRenderingSystem.h"
#include "systems/ButtonSystem.h"

#include "../RecursiveRunnerGame.h"

struct RateState::RateStateDatas {
   Entity rateText, btnNow, btnLater, btnNever;
};

RateState::RateState(RecursiveRunnerGame* game) : StateManager(State::Rate, game) {
   datas = new RateStateDatas;
}

RateState::~RateState() {
   delete datas;
}

void RateState::setup() {
   Entity entity[4];
   entity[0] = datas->rateText = theEntityManager.CreateEntity("rate_text");
   entity[1] = datas->btnNow = theEntityManager.CreateEntity("rate_button_now");
   entity[2] = datas->btnLater = theEntityManager.CreateEntity("rate_button_later");
   entity[3] = datas->btnNever = theEntityManager.CreateEntity("rate_button_never");

   for (int i = 0; i < 4; i++) {
      ADD_COMPONENT(entity[i], Transformation);
      TRANSFORM(entity[i])->z = 0.9;
      TRANSFORM(entity[i])->parent = game->cameraEntity;
      TRANSFORM(entity[i])->position = glm::vec2(0, -i);

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

void RateState::willEnter(State::Enum) {
}

void RateState::enter(State::Enum) {
   TEXT_RENDERING(datas->rateText)->show = true;
   TEXT_RENDERING(datas->btnNow)->show = true;
   BUTTON(datas->btnNow)->enabled = true;
   TEXT_RENDERING(datas->btnLater)->show = true;
   BUTTON(datas->btnLater)->enabled = true;
   TEXT_RENDERING(datas->btnNever)->show = true;
   BUTTON(datas->btnNever)->enabled = true;
}

void RateState::backgroundUpdate(float) {
}

State::Enum RateState::update(float) {
   if (BUTTON(datas->btnNow)->clicked) {
      game->gameThreadContext->communicationAPI->rateItNow();
      return State::Menu;
   } else if (BUTTON(datas->btnNever)->clicked) {
      game->gameThreadContext->communicationAPI->rateItNever();
      return State::Menu;
   } else if (BUTTON(datas->btnLater)->clicked) {
      game->gameThreadContext->communicationAPI->rateItLater();
      return State::Menu;
   }

   return State::Rate;
}

void RateState::willExit(State::Enum) {
   TEXT_RENDERING(datas->rateText)->show = false;
   TEXT_RENDERING(datas->btnNow)->show = false;
   BUTTON(datas->btnNow)->enabled = false;
   TEXT_RENDERING(datas->btnLater)->show = false;
   BUTTON(datas->btnLater)->enabled = false;
   TEXT_RENDERING(datas->btnNever)->show = false;
   BUTTON(datas->btnNever)->enabled = false;
}

void RateState::exit(State::Enum) {
}

bool RateState::transitionCanExit(State::Enum) {
   return true;
}

bool RateState::transitionCanEnter(State::Enum) {
   return true;
}
