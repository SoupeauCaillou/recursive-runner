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

#include "base/PlacementHelper.h"
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

#include "../RecursiveRunnerGame.h"
#include "../Parameters.h"

#include <sstream>
#include <vector>

static void startMenuMusic(Entity title) {
    MUSIC(title)->music = theMusicSystem.loadMusicFile("intro-menu.ogg");
    MUSIC(title)->loopNext = theMusicSystem.loadMusicFile("boucle-menu.ogg");
    MUSIC(title)->loopAt = 4.54;
    MUSIC(title)->volume = 1;
    MUSIC(title)->fadeOut = 2;
    MUSIC(title)->fadeIn = 1;
    MUSIC(title)->control = MusicControl::Play;
}

struct MenuStateManager::MenuStateManagerDatas {
    Entity titleGroup, title, subtitle, subtitleText, swarmBtn, giftizBtn;
};

MenuStateManager::MenuStateManager(RecursiveRunnerGame* game) : StateManager(State::Menu, game) {
    datas = new MenuStateManagerDatas;
}

MenuStateManager::~MenuStateManager() {
    delete datas;
}

void MenuStateManager::setup() {
    Entity titleGroup = datas->titleGroup  = theEntityManager.CreateEntity();
    ADD_COMPONENT(titleGroup, Transformation);
    TRANSFORM(titleGroup)->z = 0.7;
    TRANSFORM(titleGroup)->rotation = 0.05;
    ADD_COMPONENT(titleGroup, ADSR);
    ADSR(titleGroup)->idleValue = PlacementHelper::ScreenHeight + PlacementHelper::GimpYToScreen(400);
    ADSR(titleGroup)->sustainValue =
        game->baseLine +
        PlacementHelper::ScreenHeight - PlacementHelper::GimpSizeToScreen(theRenderingSystem.getTextureSize("titre")).Y * 0.5
        + PlacementHelper::GimpHeightToScreen(20);
    ADSR(titleGroup)->attackValue = ADSR(titleGroup)->sustainValue - PlacementHelper::GimpHeightToScreen(5);
    ADSR(titleGroup)->attackTiming = 2;
    ADSR(titleGroup)->decayTiming = 0.2;
    ADSR(titleGroup)->releaseTiming = 1.5;
    TRANSFORM(titleGroup)->position = Vector2(game->leftMostCameraPos.X + TRANSFORM(titleGroup)->size.X * 0.5, ADSR(titleGroup)->idleValue);

    Entity title = datas->title = theEntityManager.CreateEntity();
    ADD_COMPONENT(title, Transformation);
    TRANSFORM(title)->size = PlacementHelper::GimpSizeToScreen(theRenderingSystem.getTextureSize("titre"));
    TRANSFORM(title)->parent = titleGroup;
    TRANSFORM(title)->position = Vector2::Zero;
    TRANSFORM(title)->z = 0.15;
    ADD_COMPONENT(title, Rendering);
    RENDERING(title)->texture = theRenderingSystem.loadTextureFile("titre");
    RENDERING(title)->hide = false;
    RENDERING(title)->cameraBitMask = 0x1;
    ADD_COMPONENT(title, Music);

    Entity subtitle = datas->subtitle = theEntityManager.CreateEntity();
    ADD_COMPONENT(subtitle, Transformation);
    TRANSFORM(subtitle)->size = PlacementHelper::GimpSizeToScreen(theRenderingSystem.getTextureSize("taptostart"));
    TRANSFORM(subtitle)->parent = titleGroup;
    TRANSFORM(subtitle)->position = Vector2(-PlacementHelper::GimpWidthToScreen(0), -PlacementHelper::GimpHeightToScreen(150));
    TRANSFORM(subtitle)->z = -0.1;
    ADD_COMPONENT(subtitle, Rendering);
    RENDERING(subtitle)->texture = theRenderingSystem.loadTextureFile("taptostart");
    RENDERING(subtitle)->hide = false;
    RENDERING(subtitle)->cameraBitMask = 0x1;
    ADD_COMPONENT(subtitle, ADSR);
    ADSR(subtitle)->idleValue = 0;
    ADSR(subtitle)->sustainValue = -PlacementHelper::GimpHeightToScreen(150);
    ADSR(subtitle)->attackValue = -PlacementHelper::GimpHeightToScreen(150);
    ADSR(subtitle)->attackTiming = 2;
    ADSR(subtitle)->decayTiming = 0.1;
    ADSR(subtitle)->releaseTiming = 1;

    Entity subtitleText = datas->subtitleText = theEntityManager.CreateEntity();
    ADD_COMPONENT(subtitleText, Transformation);
    TRANSFORM(subtitleText)->parent = subtitle;
    TRANSFORM(subtitleText)->position = Vector2(0, -PlacementHelper::GimpHeightToScreen(25));
    ADD_COMPONENT(subtitleText, TextRendering);
    TEXT_RENDERING(subtitleText)->text = "Tap screen to start";
    TEXT_RENDERING(subtitleText)->charHeight = PlacementHelper::GimpHeightToScreen(45);
    TEXT_RENDERING(subtitleText)->hide = false;
    TEXT_RENDERING(subtitleText)->cameraBitMask = 0x1;
    TEXT_RENDERING(subtitleText)->color = Color(40.0 / 255, 32.0/255, 30.0/255, 0.8);

    Entity swarmBtn = datas->swarmBtn = theEntityManager.CreateEntity();
    ADD_COMPONENT(swarmBtn, Transformation);
    TRANSFORM(swarmBtn)->size = PlacementHelper::GimpSizeToScreen(theRenderingSystem.getTextureSize("swarm"));
    TRANSFORM(swarmBtn)->parent = game->cameraEntity;
    TRANSFORM(swarmBtn)->position =
        theRenderingSystem.cameras[0].worldSize * Vector2(-0.5, -0.5)
        + Vector2(game->buttonSpacing.H, game->buttonSpacing.V);

    TRANSFORM(swarmBtn)->z = 0.95;
    ADD_COMPONENT(swarmBtn, Rendering);
    RENDERING(swarmBtn)->texture = theRenderingSystem.loadTextureFile("swarm");
    RENDERING(swarmBtn)->hide = false;
    RENDERING(swarmBtn)->cameraBitMask = 0x1;
    ADD_COMPONENT(swarmBtn, Button);
    BUTTON(swarmBtn)->overSize = 1.2;

    Entity giftizBtn = datas->giftizBtn = theEntityManager.CreateEntity();
    ADD_COMPONENT(giftizBtn, Transformation);
    TRANSFORM(giftizBtn)->size = PlacementHelper::GimpSizeToScreen(theRenderingSystem.getTextureSize("giftiz"));
    TRANSFORM(giftizBtn)->parent = swarmBtn;
    TRANSFORM(giftizBtn)->position = Vector2(0, (TRANSFORM(swarmBtn)->size.Y) * 0.5 + game->buttonSpacing.V);
    TRANSFORM(giftizBtn)->z = 0;
    ADD_COMPONENT(giftizBtn, Rendering);
    RENDERING(giftizBtn)->texture = theRenderingSystem.loadTextureFile("giftiz");
    RENDERING(giftizBtn)->hide = false;
    RENDERING(giftizBtn)->cameraBitMask = 0x1;
    ADD_COMPONENT(giftizBtn, Button);
    BUTTON(giftizBtn)->overSize = 1.2;
}


///----------------------------------------------------------------------------//
///--------------------- ENTER SECTION ----------------------------------------//
///----------------------------------------------------------------------------//
void MenuStateManager::willEnter(State::Enum from) {
    std::vector<Entity> temp = theAutoDestroySystem.RetrieveAllEntityWithComponent();
    std::for_each(temp.begin(), temp.end(), deleteEntityFunctor);
    
    // activate animation
    ADSR(datas->titleGroup)->active = ADSR(datas->subtitle)->active = true;

    // Restore camera position
    game->setupCamera(CameraModeMenu);

    // save score if any
    std::vector<Entity> players = thePlayerSystem.RetrieveAllEntityWithComponent();
    if (!players.empty() && from == State::Game) {
        std::stringstream a;
        a << PLAYER(players[0])->score << " points - tap screen to restart";
        TEXT_RENDERING(datas->subtitleText)->text = a.str();
        game->storageAPI->submitScore(StorageAPI::Score(PLAYER(players[0])->score, PLAYER(players[0])->coins, "rzehtrtyBg"));
        game->updateBestScore();

        if (PLAYER(players[0])->score >= 15000) {
            game->communicationAPI->giftizMissionDone();
        }
    }
    // start music if not muted
    if (!theMusicSystem.isMuted()) {
        startMenuMusic(datas->title);
    }
    // unhide UI
    RENDERING(datas->swarmBtn)->hide = RENDERING(datas->giftizBtn)->hide = false;
    RENDERING(datas->swarmBtn)->color = RENDERING(datas->giftizBtn)->color = Color(1,1,1,0);

    int giftizState = game->communicationAPI->giftizGetButtonState();
    switch (giftizState) {
        case 0: 
            RENDERING(datas->giftizBtn)->hide = true;
            break;
        case 1:
            break;
        case 2:
            RENDERING(datas->giftizBtn)->texture = theRenderingSystem.loadTextureFile("giftiz_1");
            break;
        case 3:
            RENDERING(datas->giftizBtn)->texture = theRenderingSystem.loadTextureFile("giftiz_warning");
            break;
    }
}

bool MenuStateManager::transitionCanEnter(State::Enum) {
    // check if adsr is complete
    ADSRComponent* adsr = ADSR(datas->titleGroup);
    float progress = (adsr->value - adsr->idleValue) / (adsr->sustainValue - adsr->idleValue);
    RENDERING(datas->swarmBtn)->color.a = RENDERING(datas->giftizBtn)->color.a = progress;
    return (adsr->value == adsr->sustainValue);
}


void MenuStateManager::enter(State::Enum) {
    RecursiveRunnerGame::endGame();
    // enable UI
    BUTTON(datas->swarmBtn)->enabled = true;
    BUTTON(datas->giftizBtn)->enabled = true;
    RENDERING(datas->swarmBtn)->color.a = RENDERING(datas->giftizBtn)->color.a = 1;
}


///----------------------------------------------------------------------------//
///--------------------- UPDATE SECTION ---------------------------------------//
///----------------------------------------------------------------------------//
void MenuStateManager::backgroundUpdate(float) {
    TRANSFORM(datas->titleGroup)->position.Y = ADSR(datas->titleGroup)->value;
    TRANSFORM(datas->subtitle)->position.Y = ADSR(datas->subtitle)->value;
}

State::Enum MenuStateManager::update(float) {
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

    // Handle Swarm button
    if (!game->ignoreClick) {
        if (BUTTON(datas->swarmBtn)->clicked) {
            game->communicationAPI->swarmRegistering();
        }
        game->ignoreClick = BUTTON(datas->swarmBtn)->mouseOver;
    }
    RENDERING(datas->swarmBtn)->color = BUTTON(datas->swarmBtn)->mouseOver ? Color("gray") : Color();

    // Handle Giftiz button
    if (!game->ignoreClick) {
        if (BUTTON(datas->giftizBtn)->clicked) {
            game->communicationAPI->giftizButtonClicked();
        }
        game->ignoreClick = BUTTON(datas->giftizBtn)->mouseOver;
    }
    RENDERING(datas->giftizBtn)->color = BUTTON(datas->giftizBtn)->mouseOver ? Color("gray") : Color();

    // Start game ?
    if (theTouchInputManager.isTouched(0) && theTouchInputManager.wasTouched(0) && !game->ignoreClick) {
        return State::Ad;
    }
    return State::Menu;
}


///----------------------------------------------------------------------------//
///--------------------- EXIT SECTION -----------------------------------------//
///----------------------------------------------------------------------------//
void MenuStateManager::willExit(State::Enum) {
    // stop menu music
    MUSIC(datas->title)->control = MusicControl::Stop;

    // disable button interaction
    BUTTON(datas->swarmBtn)->enabled = false;
    BUTTON(datas->giftizBtn)->enabled = false;

    // activate animation
    ADSR(datas->titleGroup)->active = ADSR(datas->subtitle)->active = false;
}

bool MenuStateManager::transitionCanExit(State::Enum) {
    const ADSRComponent* adsr = ADSR(datas->titleGroup);
    float progress = (adsr->value - adsr->attackValue) /
            (adsr->idleValue - adsr->attackValue);

    RENDERING(datas->swarmBtn)->color.a = RENDERING(datas->giftizBtn)->color.a = 1 - progress;
    // check if animation is finished
    return (adsr->value >= adsr->idleValue);
}

void MenuStateManager::exit(State::Enum) {
    RENDERING(datas->swarmBtn)->hide = RENDERING(datas->giftizBtn)->hide = true;
}
