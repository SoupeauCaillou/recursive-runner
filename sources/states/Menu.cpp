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
#include "base/TouchInputManager.h"
#include "systems/TransformationSystem.h"
#include "systems/RenderingSystem.h"
#include "systems/ButtonSystem.h"
#include "systems/ADSRSystem.h"
#include "systems/TextRenderingSystem.h"
#include "systems/SoundSystem.h"
#include "systems/MusicSystem.h"
#include "systems/PlayerSystem.h"
#include "systems/ParticuleSystem.h"
#include "systems/AutoDestroySystem.h"

#include "../systems/SessionSystem.h"
#include "../systems/RunnerSystem.h"

#include "api/LocalizeAPI.h"
#include "api/StorageAPI.h"

#include "../RecursiveRunnerGame.h"
#include "../Parameters.h"

#include <sstream>
#include <vector>

static void updateGiftizButton(Entity btn, RecursiveRunnerGame* game);
static void startMenuMusic(Entity title) {
    MUSIC(title)->music = theMusicSystem.loadMusicFile("intro-menu.ogg");
    MUSIC(title)->loopNext = theMusicSystem.loadMusicFile("boucle-menu.ogg");
    MUSIC(title)->loopAt = 4.54;
    MUSIC(title)->volume = 1;
    MUSIC(title)->fadeOut = 2;
    MUSIC(title)->fadeIn = 1;
    MUSIC(title)->control = MusicControl::Play;
}

struct MenuState::MenuStateDatas {
    Entity titleGroup, title, subtitle, subtitleText, helpBtn, goToSocialCenterBtn, giftizBtn;
};

MenuState::MenuState(RecursiveRunnerGame* game) : StateManager(State::Menu, game) {
    datas = new MenuStateDatas;
}

MenuState::~MenuState() {
    delete datas;
}

void MenuState::setup() {
    Entity titleGroup = datas->titleGroup  = theEntityManager.CreateEntity("title_group");
    ADD_COMPONENT(titleGroup, Transformation);
    TRANSFORM(titleGroup)->z = 0.7;
    TRANSFORM(titleGroup)->rotation = 0.02;
    ADD_COMPONENT(titleGroup, ADSR);
    ADSR(titleGroup)->idleValue = PlacementHelper::ScreenHeight + PlacementHelper::GimpYToScreen(400);
    ADSR(titleGroup)->sustainValue =
        game->baseLine +
        PlacementHelper::ScreenHeight - PlacementHelper::GimpSizeToScreen(theRenderingSystem.getTextureSize("titre")).y * 0.5
        + PlacementHelper::GimpHeightToScreen(10);
    ADSR(titleGroup)->attackValue = ADSR(titleGroup)->sustainValue - PlacementHelper::GimpHeightToScreen(5);
    ADSR(titleGroup)->attackTiming = 2;
    ADSR(titleGroup)->decayTiming = 0.2;
    ADSR(titleGroup)->releaseTiming = 1.5;
    TRANSFORM(titleGroup)->position = glm::vec2(game->leftMostCameraPos.x + TRANSFORM(titleGroup)->size.x * 0.5, ADSR(titleGroup)->idleValue);

    Entity title = datas->title = theEntityManager.CreateEntity("title");
    ADD_COMPONENT(title, Transformation);
    TRANSFORM(title)->size = PlacementHelper::GimpSizeToScreen(theRenderingSystem.getTextureSize("titre"));
    TRANSFORM(title)->parent = titleGroup;
    TRANSFORM(title)->position = glm::vec2(0.0f);
    TRANSFORM(title)->z = 0.15;
    ADD_COMPONENT(title, Rendering);
    RENDERING(title)->texture = theRenderingSystem.loadTextureFile("titre");
    RENDERING(title)->show = true;
    ADD_COMPONENT(title, Music);

    Entity subtitle = datas->subtitle = theEntityManager.CreateEntity("subtitle");
    ADD_COMPONENT(subtitle, Transformation);
    TRANSFORM(subtitle)->size = PlacementHelper::GimpSizeToScreen(theRenderingSystem.getTextureSize("taptostart"));
    TRANSFORM(subtitle)->parent = titleGroup;
    TRANSFORM(subtitle)->position = glm::vec2(-PlacementHelper::GimpWidthToScreen(0), -PlacementHelper::GimpHeightToScreen(150));
    TRANSFORM(subtitle)->z = -0.1;
    ADD_COMPONENT(subtitle, Rendering);
    RENDERING(subtitle)->texture = theRenderingSystem.loadTextureFile("taptostart");
    RENDERING(subtitle)->show = true;
    ADD_COMPONENT(subtitle, ADSR);
    ADSR(subtitle)->idleValue = 0;
    ADSR(subtitle)->sustainValue = -PlacementHelper::GimpHeightToScreen(150);
    ADSR(subtitle)->attackValue = -PlacementHelper::GimpHeightToScreen(150);
    ADSR(subtitle)->attackTiming = 2;
    ADSR(subtitle)->decayTiming = 0.1;
    ADSR(subtitle)->releaseTiming = 1;

    Entity subtitleText = datas->subtitleText = theEntityManager.CreateEntity("subtitle_text");
    ADD_COMPONENT(subtitleText, Transformation);
    TRANSFORM(subtitleText)->parent = subtitle;
    TRANSFORM(subtitleText)->z = 0.01;
    TRANSFORM(subtitleText)->rotation = 0.005;
    TRANSFORM(subtitleText)->position = glm::vec2(0, -PlacementHelper::GimpHeightToScreen(25));
    TRANSFORM(subtitleText)->size = glm::vec2(PlacementHelper::GimpWidthToScreen(790), 1);
    ADD_COMPONENT(subtitleText, TextRendering);
    TEXT_RENDERING(subtitleText)->text = game->gameThreadContext->localizeAPI->text("tap_screen_to_start");
    TEXT_RENDERING(subtitleText)->charHeight = 1.5 * PlacementHelper::GimpHeightToScreen(45);
    TEXT_RENDERING(subtitleText)->show = true;
    TEXT_RENDERING(subtitleText)->color = Color(40.0 / 255, 32.0/255, 30.0/255, 0.8);
    TEXT_RENDERING(subtitleText)->flags = TextRenderingComponent::AdjustHeightToFillWidthBit;

    Entity goToSocialCenterBtn = datas->goToSocialCenterBtn = theEntityManager.CreateEntity("goToSocialCenter_button");
    ADD_COMPONENT(goToSocialCenterBtn, Transformation);
    TRANSFORM(goToSocialCenterBtn)->size = PlacementHelper::GimpSizeToScreen(theRenderingSystem.getTextureSize("swarm"));
    TRANSFORM(goToSocialCenterBtn)->parent = game->cameraEntity;
    TRANSFORM(goToSocialCenterBtn)->position =
        TRANSFORM(game->cameraEntity)->size * glm::vec2(-0.5, -0.5)
        + glm::vec2(game->buttonSpacing.H, game->buttonSpacing.V);
    TRANSFORM(goToSocialCenterBtn)->z = 0.95;
    ADD_COMPONENT(goToSocialCenterBtn, Rendering);
    RENDERING(goToSocialCenterBtn)->texture = theRenderingSystem.loadTextureFile("swarm");
    RENDERING(goToSocialCenterBtn)->show = true;
    ADD_COMPONENT(goToSocialCenterBtn, Button);
    BUTTON(goToSocialCenterBtn)->overSize = 1.2;

    Entity giftizBtn = datas->giftizBtn = theEntityManager.CreateEntity("giftiz_button");
    ADD_COMPONENT(giftizBtn, Transformation);
    TRANSFORM(giftizBtn)->size = PlacementHelper::GimpSizeToScreen(theRenderingSystem.getTextureSize("giftiz"));
    TRANSFORM(giftizBtn)->parent = goToSocialCenterBtn;
    TRANSFORM(giftizBtn)->position = glm::vec2(0, (TRANSFORM(goToSocialCenterBtn)->size.y) * 0.5 + game->buttonSpacing.V);
    TRANSFORM(giftizBtn)->z = 0;
    ADD_COMPONENT(giftizBtn, Rendering);
    RENDERING(giftizBtn)->texture = theRenderingSystem.loadTextureFile("giftiz");
    RENDERING(giftizBtn)->show = true;
    ADD_COMPONENT(giftizBtn, Button);
    BUTTON(giftizBtn)->overSize = 1.2;

    Entity helpBtn = datas->helpBtn = theEntityManager.CreateEntity("help_button");
    ADD_COMPONENT(helpBtn, Transformation);
    TRANSFORM(helpBtn)->size = PlacementHelper::GimpSizeToScreen(theRenderingSystem.getTextureSize("aide"));
    TRANSFORM(helpBtn)->parent = game->muteBtn;
    TRANSFORM(helpBtn)->position = glm::vec2(0, -(TRANSFORM(helpBtn)->size.y * 0.5 + game->buttonSpacing.V));
    TRANSFORM(helpBtn)->z = 0;
    ADD_COMPONENT(helpBtn, Rendering);
    RENDERING(helpBtn)->texture = theRenderingSystem.loadTextureFile("aide");
    RENDERING(helpBtn)->show = true;
    ADD_COMPONENT(helpBtn, Button);
    BUTTON(helpBtn)->overSize = 1.2;
}


///----------------------------------------------------------------------------//
///--------------------- ENTER SECTION ----------------------------------------//
///----------------------------------------------------------------------------//
void MenuState::willEnter(State::Enum from) {
    std::vector<Entity> temp = theAutoDestroySystem.RetrieveAllEntityWithComponent();
    std::for_each(temp.begin(), temp.end(), deleteEntityFunctor);

    // activate animation
    ADSR(datas->titleGroup)->active = ADSR(datas->subtitle)->active = true;

    // Restore camera position
    game->setupCamera(CameraMode::Menu);

    // save score if any
    std::vector<Entity> players = thePlayerSystem.RetrieveAllEntityWithComponent();
    if (!players.empty() && from == State::Game) {
        std::stringstream a;
        a << PLAYER(players[0])->score << " points - " << game->gameThreadContext->localizeAPI->text("tap_screen_to_restart");
        TEXT_RENDERING(datas->subtitleText)->text = a.str();
        game->recursiveRunnerStorageAPI->submitScore(RecursiveRunnerStorageAPI::Score(PLAYER(players[0])->score, PLAYER(players[0])->coins, "rzehtrtyBg"));
        game->updateBestScore();

        if (PLAYER(players[0])->score >= 15000) {
            game->gameThreadContext->communicationAPI->giftizMissionDone();
        }
    }
    // start music if not muted
    if (!theMusicSystem.isMuted() && MUSIC(datas->title)->control == MusicControl::Stop) {
        startMenuMusic(datas->title);
    }
    // unhide UI
    RENDERING(datas->goToSocialCenterBtn)->show = RENDERING(datas->giftizBtn)->show = RENDERING(datas->helpBtn)->show = true;
    RENDERING(datas->goToSocialCenterBtn)->color = RENDERING(datas->giftizBtn)->color = RENDERING(datas->helpBtn)->color = Color(1,1,1,0);
    updateGiftizButton(datas->giftizBtn, game);
}

bool MenuState::transitionCanEnter(State::Enum) {
    // check if adsr is complete
    ADSRComponent* adsr = ADSR(datas->titleGroup);
    float progress = (adsr->value - adsr->idleValue) / (adsr->sustainValue - adsr->idleValue);
    RENDERING(datas->goToSocialCenterBtn)->color.a = RENDERING(datas->giftizBtn)->color.a = RENDERING(datas->helpBtn)->color.a = progress;
    return (adsr->value == adsr->sustainValue);
}


void MenuState::enter(State::Enum) {
    RecursiveRunnerGame::endGame();
    // enable UI
    BUTTON(datas->goToSocialCenterBtn)->enabled = BUTTON(datas->giftizBtn)->enabled = BUTTON(datas->helpBtn)->enabled = true;
    RENDERING(datas->goToSocialCenterBtn)->color.a = RENDERING(datas->giftizBtn)->color.a = RENDERING(datas->helpBtn)->color.a = 1;
}


///----------------------------------------------------------------------------//
///--------------------- UPDATE SECTION ---------------------------------------//
///----------------------------------------------------------------------------//
void MenuState::backgroundUpdate(float) {
    TRANSFORM(datas->titleGroup)->position.y = ADSR(datas->titleGroup)->value;
    TRANSFORM(datas->subtitle)->position.y = ADSR(datas->subtitle)->value;
}

State::Enum MenuState::update(float) {
    // Menu music
    if (!theMusicSystem.isMuted()) {
        MusicComponent* music = MUSIC(datas->title);
        music->control = MusicControl::Play;

        if (music->music == InvalidMusicRef) {
            startMenuMusic(datas->title);
        } else if (music->loopNext == InvalidMusicRef) {
            music->loopAt = 21.34;
            music->loopNext = theMusicSystem.loadMusicFile("boucle-menu.ogg");
        }
    } else if (theMusicSystem.isMuted() != theSoundSystem.mute) {
        // restore music
        if (theTouchInputManager.isTouched(0)) {
            theMusicSystem.toggleMute(theSoundSystem.mute);
            game->ignoreClick = true;
            startMenuMusic(datas->title);
        }
    }
    updateGiftizButton(datas->giftizBtn, game);

    // Handle SocialCenter button
    if (!game->ignoreClick) {
        if (BUTTON(datas->goToSocialCenterBtn)->clicked) {
            return State::SocialCenter;
            //game->gameThreadContext->communicationAPI->openGameCenter();
        }
        game->ignoreClick = BUTTON(datas->goToSocialCenterBtn)->mouseOver;
    }
    RENDERING(datas->goToSocialCenterBtn)->color = BUTTON(datas->goToSocialCenterBtn)->mouseOver ? Color("gray") : Color();

    // Handle Giftiz button
    if (!game->ignoreClick) {
        if (BUTTON(datas->giftizBtn)->clicked) {
            game->gameThreadContext->communicationAPI->giftizButtonClicked();
        }
        game->ignoreClick = BUTTON(datas->giftizBtn)->mouseOver;
    }
    RENDERING(datas->giftizBtn)->color = BUTTON(datas->giftizBtn)->mouseOver ? Color("gray") : Color();

    // Handle help button
    if (!game->ignoreClick) {
        if (BUTTON(datas->helpBtn)->clicked) {
            return State::Tutorial;
        }
        game->ignoreClick = BUTTON(datas->helpBtn)->mouseOver;
    }
    RENDERING(datas->helpBtn)->color = BUTTON(datas->helpBtn)->mouseOver ? Color("gray") : Color();

    // Start game ? (tutorial if no game done)
    if (theTouchInputManager.isTouched(0) && theTouchInputManager.wasTouched(0) && !game->ignoreClick) {
        return State::Ad;
        #if 0
        game->gameThreadContext->storageAPI->incrementGameCount();
        if (game->gameThreadContext->storageAPI->isFirstGame()) {
            return State::Tutorial;
        } else {
            return State::Ad;
        }
        #endif
    }
    return State::Menu;
}


///----------------------------------------------------------------------------//
///--------------------- EXIT SECTION -----------------------------------------//
///----------------------------------------------------------------------------//
void MenuState::willExit(State::Enum to) {
    if (to != State::SocialCenter) {
        // stop menu music
        MUSIC(datas->title)->control = MusicControl::Stop;
    }

    // disable button interaction
    BUTTON(datas->goToSocialCenterBtn)->enabled = false;
    BUTTON(datas->giftizBtn)->enabled = false;
    BUTTON(datas->helpBtn)->enabled = false;

    // activate animation
    ADSR(datas->titleGroup)->active = ADSR(datas->subtitle)->active = false;
}

bool MenuState::transitionCanExit(State::Enum) {
    const ADSRComponent* adsr = ADSR(datas->titleGroup);
    float progress = (adsr->value - adsr->attackValue) /
            (adsr->idleValue - adsr->attackValue);

    RENDERING(datas->goToSocialCenterBtn)->color.a = RENDERING(datas->giftizBtn)->color.a = RENDERING(datas->helpBtn)->color.a = 1 - progress;
    // check if animation is finished
    return (adsr->value >= adsr->idleValue);
}

void MenuState::exit(State::Enum) {
    RENDERING(datas->goToSocialCenterBtn)->show = RENDERING(datas->giftizBtn)->show = RENDERING(datas->helpBtn)->show = false;
}

#ifndef ANDROID
static void updateGiftizButton(Entity btn, RecursiveRunnerGame*) {
    RENDERING(btn)->show = true;
#else
static void updateGiftizButton(Entity btn, RecursiveRunnerGame* game) {
     int giftizState = game->communicationAPI->giftizGetButtonState();
    switch (giftizState) {
        case 0:
            RENDERING(btn)->show = false;
            break;
        case 1:
            RENDERING(btn)->show = true;
            RENDERING(btn)->texture = theRenderingSystem.loadTextureFile("giftiz");
            break;
        case 2:
            RENDERING(btn)->show = true;
            RENDERING(btn)->texture = theRenderingSystem.loadTextureFile("giftiz_1");
            break;
        case 3:
            RENDERING(btn)->show = true;
            RENDERING(btn)->texture = theRenderingSystem.loadTextureFile("giftiz_warning");
            break;
    }
#endif
}
