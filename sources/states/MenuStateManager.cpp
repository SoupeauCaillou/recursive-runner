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
        + TRANSFORM(swarmBtn)->size * Vector2(0.5, 0.5)
        + Vector2(0, game->baseLine + theRenderingSystem.cameras[0].worldSize.Y * 0.5);

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
    TRANSFORM(giftizBtn)->position = Vector2(0, TRANSFORM(giftizBtn)->size.Y * 1.2);
    TRANSFORM(giftizBtn)->z = 0;
    ADD_COMPONENT(giftizBtn, Rendering);
    RENDERING(giftizBtn)->texture = theRenderingSystem.loadTextureFile("giftiz");
    RENDERING(giftizBtn)->hide = false;
    RENDERING(giftizBtn)->cameraBitMask = 0x1;
    ADD_COMPONENT(giftizBtn, Button);
    BUTTON(giftizBtn)->overSize = 1.2;
}

void MenuStateManager::willEnter() {
    // Restore camera position
    for (unsigned i=0; i<theRenderingSystem.cameras.size(); i++) {
        theRenderingSystem.cameras[i].worldPosition = game->leftMostCameraPos;
    }
    game->setupCamera(CameraModeMenu);
    std::vector<Entity> players = thePlayerSystem.RetrieveAllEntityWithComponent();
    if (!players.empty()) {
        std::stringstream a;
        a << PLAYER(players[0])->score << " points - tap screen to restart";
        TEXT_RENDERING(datas->subtitleText)->text = a.str();
        game->storageAPI->submitScore(StorageAPI::Score(PLAYER(players[0])->score, PLAYER(players[0])->coins, "rzehtrtyBg"));
        game->updateBestScore();
    }
    if (!theMusicSystem.isMuted()) {
        startMenuMusic(datas->title);
    }
}

void MenuStateManager::enter() {
    RecursiveRunnerGame::endGame();
    BUTTON(datas->swarmBtn)->enabled = true;
    BUTTON(datas->giftizBtn)->enabled = true;
}

void MenuStateManager::backgroundUpdate(float) {
    TRANSFORM(datas->titleGroup)->position.Y = ADSR(datas->titleGroup)->value;
    TRANSFORM(datas->subtitle)->position.Y = ADSR(datas->subtitle)->value;
}

State::Enum MenuStateManager::update(float) {
    const Entity titleGroup = datas->titleGroup;
    const Entity title = datas->title;
    const Entity subtitle = datas->subtitle;

    if (!theMusicSystem.isMuted()) {
        MusicComponent* music = MUSIC(title);
        music->control = MusicControl::Play;

        if (music->music == InvalidMusicRef) {
            startMenuMusic(title);
        } else if (music->loopNext == InvalidMusicRef) {
            music->loopAt = 21.34;
            music->loopNext = theMusicSystem.loadMusicFile("boucle-menu.ogg");
        }
    }

    // Handle Swarm button
    if (!game->ignoreClick) {
        if (BUTTON(datas->swarmBtn)->clicked) {
            game->communicationAPI->swarmRegistering();
        }
        game->ignoreClick = BUTTON(datas->swarmBtn)->mouseOver;
    }

    // Handle Giftiz button
    if (!game->ignoreClick) {
        if (BUTTON(datas->giftizBtn)->clicked) {
            game->communicationAPI->giftizButtonClicked();
        }
        game->ignoreClick = BUTTON(datas->giftizBtn)->mouseOver;
    }

    // Restore music ?
    if (theTouchInputManager.isTouched(0)) {
        if (theMusicSystem.isMuted() != theSoundSystem.mute) {
            theMusicSystem.toggleMute(theSoundSystem.mute);
            game->ignoreClick = true;
        }
    }

    // Start game ?
    if (theTouchInputManager.isTouched(0) && theTouchInputManager.wasTouched(0) && !game->ignoreClick) {
        ADSR(titleGroup)->active = ADSR(subtitle)->active = false;
        MUSIC(game->route)->music = theMusicSystem.loadMusicFile("jeu.ogg");
        return State::Ad;
    }
    return State::Menu;
}

void MenuStateManager::willExit() {
    RecursiveRunnerGame::startGame(true);

    MUSIC(datas->title)->control = MusicControl::Stop;
    BUTTON(datas->swarmBtn)->enabled = false;
    BUTTON(datas->giftizBtn)->enabled = false;
}

void MenuStateManager::exit() {
}

bool MenuStateManager::transitionCanExit() {
    const SessionComponent* session = SESSION(theSessionSystem.RetrieveAllEntityWithComponent().front());
    const ADSRComponent* adsr = ADSR(datas->titleGroup);
    {
        float progress = (adsr->value - adsr->attackValue) /
            (adsr->idleValue - adsr->attackValue);
        progress = MathUtil::Max(0.0f, MathUtil::Min(1.0f, progress));
        for (unsigned i=0; i<session->coins.size(); i++) {
            RENDERING(session->coins[i])->color.a = progress;
        }
        for (unsigned i=0; i<session->links.size(); i++) {
            if (i % 2)
                RENDERING(session->links[i])->color.a = progress * 0.2;
            else
                RENDERING(session->links[i])->color.a = progress;
        }
    }
    PLAYER(session->players[0])->ready = true;
    if (adsr->value < adsr->idleValue) {
        return false;
    }
    return true;
}

bool MenuStateManager::transitionCanEnter() {
    ADSRComponent* adsr = ADSR(datas->titleGroup);
    adsr->active = ADSR(datas->subtitle)->active = true;
    std::vector<Entity> sessions = theSessionSystem.RetrieveAllEntityWithComponent();
    if (!sessions.empty()) {
        const SessionComponent* session = SESSION(sessions.front());
        {
            float progress = (adsr->value - adsr->attackValue) /
                (adsr->idleValue - adsr->attackValue);
            progress = MathUtil::Max(0.0f, MathUtil::Min(1.0f, progress));
            for (unsigned i=0; i<session->coins.size(); i++) {
                RENDERING(session->coins[i])->color.a = progress;
            }
            for (unsigned i=0; i<session->links.size(); i++) {
                if (i % 2)
                    RENDERING(session->links[i])->color.a = progress * 0.2;
                else
                    RENDERING(session->links[i])->color.a = progress;
            }
        }
    }

    return (adsr->value == adsr->sustainValue);
}
