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

#include <base/EntityManager.h>

#include <systems/TransformationSystem.h>
#include <systems/ButtonSystem.h>
#include <systems/RenderingSystem.h>

#include "RecursiveRunnerGame.h"
#include <api/CommunicationAPI.h>

struct SocialCenterState::SocialCenterStateDatas {
    Entity menuBtn;
};

SocialCenterState::SocialCenterState(RecursiveRunnerGame* game) : StateManager(State::SocialCenter, game) {
    datas = new SocialCenterStateDatas;
}

SocialCenterState::~SocialCenterState() {
    delete datas;
}

void SocialCenterState::setup() {
    Entity menuBtn = datas->menuBtn = theEntityManager.CreateEntity("menuBtn");
    ADD_COMPONENT(menuBtn, Transformation);
    TRANSFORM(menuBtn)->z = .9;
    TRANSFORM(menuBtn)->position = glm::vec2(9., -5);
    ADD_COMPONENT(menuBtn, Button);
    ADD_COMPONENT(menuBtn, Rendering);
    RENDERING(menuBtn)->color = Color::random();
}



///----------------------------------------------------------------------------//
///--------------------- ENTER SECTION ----------------------------------------//
///----------------------------------------------------------------------------//
void SocialCenterState::willEnter(State::Enum) {
    if (game->gameThreadContext->communicationAPI != 0) {
        /*for (Score::Struct score : game->gameThreadContext->communicationAPI->getScores(0, Score::ALL, 1, 10)) {
            LOGI(score);
        }*/
    }
}

bool SocialCenterState::transitionCanEnter(State::Enum) {
    return true;
}


void SocialCenterState::enter(State::Enum) {
    BUTTON(datas->menuBtn)->enabled =
    RENDERING(datas->menuBtn)->show = true;
}


///----------------------------------------------------------------------------//
///--------------------- UPDATE SECTION ---------------------------------------//
///----------------------------------------------------------------------------//
void SocialCenterState::backgroundUpdate(float) {
}

State::Enum SocialCenterState::update(float) {
    if (BUTTON(datas->menuBtn)->clicked)
        return State::Menu;
    return State::SocialCenter;
}


///----------------------------------------------------------------------------//
///--------------------- EXIT SECTION -----------------------------------------//
///----------------------------------------------------------------------------//
void SocialCenterState::willExit(State::Enum) {

}

bool SocialCenterState::transitionCanExit(State::Enum) {
    return true;
}

void SocialCenterState::exit(State::Enum) {
    BUTTON(datas->menuBtn)->enabled =
    RENDERING(datas->menuBtn)->show = false;
}
