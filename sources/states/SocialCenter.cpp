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

#include <sstream>
#include <vector>

#include "base/EntityManager.h"
#include "base/PlacementHelper.h"

#include <systems/TransformationSystem.h>
#include <systems/ButtonSystem.h>
#include <systems/RenderingSystem.h>

#include "RecursiveRunnerGame.h"
#include <api/CommunicationAPI.h>

struct SocialCenterState::SocialCenterStateDatas {
    Entity goToMenuBtn;
};

SocialCenterState::SocialCenterState(RecursiveRunnerGame* game) : StateManager(State::SocialCenter, game) {
    datas = new SocialCenterStateDatas;
}

SocialCenterState::~SocialCenterState() {
    delete datas;
}

void SocialCenterState::setup() {
    Entity goToMenuBtn = datas->goToMenuBtn = theEntityManager.CreateEntity("goToMenu_button");
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
void SocialCenterState::willEnter(State::Enum) {
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
    RENDERING(datas->goToMenuBtn)->show = true;
}

bool SocialCenterState::transitionCanEnter(State::Enum) {
    return true;
}


void SocialCenterState::enter(State::Enum) {
    BUTTON(datas->goToMenuBtn)->enabled = true;
}


///----------------------------------------------------------------------------//
///--------------------- UPDATE SECTION ---------------------------------------//
///----------------------------------------------------------------------------//
void SocialCenterState::backgroundUpdate(float) {
}

State::Enum SocialCenterState::update(float) {
    if (BUTTON(datas->goToMenuBtn)->clicked)
        return State::Menu;
    return State::SocialCenter;
}


///----------------------------------------------------------------------------//
///--------------------- EXIT SECTION -----------------------------------------//
///----------------------------------------------------------------------------//
void SocialCenterState::willExit(State::Enum) {
    BUTTON(datas->goToMenuBtn)->enabled = false;
}

bool SocialCenterState::transitionCanExit(State::Enum) {
    return true;
}

void SocialCenterState::exit(State::Enum) {
    RENDERING(datas->goToMenuBtn)->show = false;
}
