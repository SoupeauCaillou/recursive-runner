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

static void updateTitleSubTitle(Entity title, Entity subtitle);
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

class MenuScene : public StateHandler<Scene::Enum> {
    RecursiveRunnerGame* game;
    Entity titleGroup, title, subtitle, subtitleText, helpBtn, goToSocialCenterBtn, giftizBtn;

    public:
        MenuScene(RecursiveRunnerGame* game) : StateHandler<Scene::Enum>() {
            this->game = game;
        }


        void setup() {
            titleGroup  = theEntityManager.CreateEntity("title_group");
            ADD_COMPONENT(titleGroup, Transformation);
            TRANSFORM(titleGroup)->z = 0.7;
            TRANSFORM(titleGroup)->rotation = 0.02;
            ADD_COMPONENT(titleGroup, ADSR);
            ADSR(titleGroup)->idleValue = PlacementHelper::ScreenSize.y + PlacementHelper::GimpYToScreen(400);
            ADSR(titleGroup)->sustainValue =
                game->baseLine +
                PlacementHelper::ScreenSize.y - PlacementHelper::GimpSizeToScreen(theRenderingSystem.getTextureSize("titre")).y * 0.5
                + PlacementHelper::GimpHeightToScreen(10);
            ADSR(titleGroup)->attackValue = ADSR(titleGroup)->sustainValue - PlacementHelper::GimpHeightToScreen(5);
            ADSR(titleGroup)->attackTiming = 2;
            ADSR(titleGroup)->decayTiming = 0.2;
            ADSR(titleGroup)->releaseTiming = 1.5;
            TRANSFORM(titleGroup)->position = glm::vec2(game->leftMostCameraPos.x + TRANSFORM(titleGroup)->size.x * 0.5, ADSR(titleGroup)->idleValue);

            title = theEntityManager.CreateEntity("title");
            ADD_COMPONENT(title, Transformation);
            TRANSFORM(title)->size = PlacementHelper::GimpSizeToScreen(theRenderingSystem.getTextureSize("titre"));
            ADD_COMPONENT(title, Anchor);
            ANCHOR(title)->parent = titleGroup;
            ANCHOR(title)->position = glm::vec2(0.0f);
            ANCHOR(title)->z = 0.15;
            ADD_COMPONENT(title, Rendering);
            RENDERING(title)->texture = theRenderingSystem.loadTextureFile("titre");
            RENDERING(title)->show = true;
            ADD_COMPONENT(title, Music);

            subtitle = theEntityManager.CreateEntity("subtitle");
            ADD_COMPONENT(subtitle, Transformation);
            TRANSFORM(subtitle)->size = PlacementHelper::GimpSizeToScreen(theRenderingSystem.getTextureSize("taptostart"));
            ADD_COMPONENT(subtitle, Anchor);
            ANCHOR(subtitle)->parent = titleGroup;
            ANCHOR(subtitle)->position = glm::vec2(-PlacementHelper::GimpWidthToScreen(0), -PlacementHelper::GimpHeightToScreen(150));
            ANCHOR(subtitle)->z = -0.1;
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

            subtitleText = theEntityManager.CreateEntity("subtitle_text");
            ADD_COMPONENT(subtitleText, Transformation);
            TRANSFORM(subtitleText)->size = glm::vec2(PlacementHelper::GimpWidthToScreen(790), 1);
            ADD_COMPONENT(subtitleText, Anchor);
            ANCHOR(subtitleText)->parent = subtitle;
            ANCHOR(subtitleText)->z = 0.01;
            ANCHOR(subtitleText)->rotation = 0.005;
            ANCHOR(subtitleText)->position = glm::vec2(0, -PlacementHelper::GimpHeightToScreen(25));
            ADD_COMPONENT(subtitleText, Text);
            TEXT(subtitleText)->text = game->gameThreadContext->localizeAPI->text("tap_screen_to_start");
            TEXT(subtitleText)->charHeight = 1.5 * PlacementHelper::GimpHeightToScreen(45);
            TEXT(subtitleText)->show = true;
            TEXT(subtitleText)->color = Color(40.0 / 255, 32.0/255, 30.0/255, 0.8);
            TEXT(subtitleText)->flags = TextComponent::AdjustHeightToFillWidthBit;

            goToSocialCenterBtn = theEntityManager.CreateEntity("goToSocialCenter_button");
            ADD_COMPONENT(goToSocialCenterBtn, Transformation);
            TRANSFORM(goToSocialCenterBtn)->size = PlacementHelper::GimpSizeToScreen(theRenderingSystem.getTextureSize("swarm"));
            ADD_COMPONENT(goToSocialCenterBtn, Anchor);
            ANCHOR(goToSocialCenterBtn)->parent = game->cameraEntity;
            ANCHOR(goToSocialCenterBtn)->position =
                TRANSFORM(game->cameraEntity)->size * glm::vec2(-0.5, -0.5)
                + glm::vec2(game->buttonSpacing.H, game->buttonSpacing.V);
            ANCHOR(goToSocialCenterBtn)->z = 0.95;
            ADD_COMPONENT(goToSocialCenterBtn, Rendering);
            RENDERING(goToSocialCenterBtn)->texture = theRenderingSystem.loadTextureFile("swarm");
            RENDERING(goToSocialCenterBtn)->show = true;
            ADD_COMPONENT(goToSocialCenterBtn, Button);
            BUTTON(goToSocialCenterBtn)->overSize = 1.2;

            giftizBtn = theEntityManager.CreateEntity("giftiz_button");
            ADD_COMPONENT(giftizBtn, Transformation);
            TRANSFORM(giftizBtn)->size = PlacementHelper::GimpSizeToScreen(theRenderingSystem.getTextureSize("giftiz"));
            ADD_COMPONENT(giftizBtn, Anchor);
            ANCHOR(giftizBtn)->parent = goToSocialCenterBtn;
            ANCHOR(giftizBtn)->position = glm::vec2(0, (TRANSFORM(goToSocialCenterBtn)->size.y) * 0.5 + game->buttonSpacing.V);
            ANCHOR(giftizBtn)->z = 0;
            ADD_COMPONENT(giftizBtn, Rendering);
            RENDERING(giftizBtn)->texture = theRenderingSystem.loadTextureFile("giftiz");
            RENDERING(giftizBtn)->show = true;
            ADD_COMPONENT(giftizBtn, Button);
            BUTTON(giftizBtn)->overSize = 1.2;

            helpBtn = theEntityManager.CreateEntity("help_button");
            ADD_COMPONENT(helpBtn, Transformation);
            TRANSFORM(helpBtn)->size = PlacementHelper::GimpSizeToScreen(theRenderingSystem.getTextureSize("aide"));
            ADD_COMPONENT(helpBtn, Anchor);
            ANCHOR(helpBtn)->parent = game->muteBtn;
            ANCHOR(helpBtn)->position = glm::vec2(0, -(TRANSFORM(helpBtn)->size.y * 0.5 + game->buttonSpacing.V));
            ANCHOR(helpBtn)->z = 0;
            ADD_COMPONENT(helpBtn, Rendering);
            RENDERING(helpBtn)->texture = theRenderingSystem.loadTextureFile("aide");
            RENDERING(helpBtn)->show = true;
            ADD_COMPONENT(helpBtn, Button);
            BUTTON(helpBtn)->overSize = 1.2;
        }


        ///----------------------------------------------------------------------------//
        ///--------------------- ENTER SECTION ----------------------------------------//
        ///----------------------------------------------------------------------------//
        void onPreEnter(Scene::Enum from) {
            std::vector<Entity> temp = theAutoDestroySystem.RetrieveAllEntityWithComponent();
            std::for_each(temp.begin(), temp.end(), deleteEntityFunctor);

            // activate animation
            ADSR(titleGroup)->active = ADSR(subtitle)->active = true;

            // Restore camera position
            game->setupCamera(CameraMode::Menu);

            // save score if any
            std::vector<Entity> players = thePlayerSystem.RetrieveAllEntityWithComponent();
            if (!players.empty() && from == Scene::Game) {
                TEXT(subtitleText)->text = ObjectSerializer<int>::object2string(PLAYER(players[0])->points)
                + " points - " + game->gameThreadContext->localizeAPI->text("tap_screen_to_restart");

                //save the score in DB
                ScoreStorageProxy ssp;
                ssp.setValue("points", ObjectSerializer<int>::object2string(PLAYER(players[0])->points), true); // ask to create a new score
                ssp.setValue("coins", ObjectSerializer<int>::object2string(PLAYER(players[0])->coins), false);
                ssp.setValue("name", "rzehtrtyBg", false);
                game->gameThreadContext->storageAPI->saveEntries(&ssp);

                game->updateBestScore();

                if (PLAYER(players[0])->points >= 15000) {
                    game->gameThreadContext->communicationAPI->giftizMissionDone();
                }
            }
            // start music if not muted
            if (!theMusicSystem.isMuted() && MUSIC(title)->control == MusicControl::Stop) {
                startMenuMusic(title);
            }
            // unhide UI
            RENDERING(goToSocialCenterBtn)->show = RENDERING(giftizBtn)->show = RENDERING(helpBtn)->show = true;
            RENDERING(goToSocialCenterBtn)->color = RENDERING(giftizBtn)->color = RENDERING(helpBtn)->color = Color(1,1,1,0);
            updateGiftizButton(giftizBtn, game);
        }

        bool updatePreEnter(Scene::Enum, float) {
            updateTitleSubTitle(titleGroup, subtitle);
            // check if adsr is complete
            ADSRComponent* adsr = ADSR(titleGroup);
            float progress = (adsr->value - adsr->idleValue) / (adsr->sustainValue - adsr->idleValue);
            RENDERING(goToSocialCenterBtn)->color.a = RENDERING(giftizBtn)->color.a = RENDERING(helpBtn)->color.a = progress;
            return (adsr->value == adsr->sustainValue);
        }


        void onEnter(Scene::Enum) {
            RecursiveRunnerGame::endGame();
            // enable UI
            BUTTON(goToSocialCenterBtn)->enabled = BUTTON(giftizBtn)->enabled = BUTTON(helpBtn)->enabled = true;
            RENDERING(goToSocialCenterBtn)->color.a = RENDERING(giftizBtn)->color.a = RENDERING(helpBtn)->color.a = 1;
        }

#if 0
void backgroundUpdate(float) {
    TRANSFORM(datas->titleGroup)->position.y = ADSR(datas->titleGroup)->value;
    TRANSFORM(datas->subtitle)->position.y = ADSR(datas->subtitle)->value;
}
#endif

        Scene::Enum update(float) {
            // Menu music
            if (!theMusicSystem.isMuted()) {
                MusicComponent* music = MUSIC(title);
                music->control = MusicControl::Play;

                if (music->music == InvalidMusicRef) {
                    startMenuMusic(title);
                } else if (music->loopNext == InvalidMusicRef) {
                    music->loopAt = 21.34;
                    music->loopNext = theMusicSystem.loadMusicFile("boucle-menu.ogg");
                }
            } else if (theMusicSystem.isMuted() != theSoundSystem.mute) {
                // restore music
                if (theTouchInputManager.isTouched(0)) {
                    theMusicSystem.toggleMute(theSoundSystem.mute);
                    game->ignoreClick = true;
                    startMenuMusic(title);
                }
            }
            updateGiftizButton(giftizBtn, game);

            // Handle Giftiz button
            if (!game->ignoreClick) {
                if (BUTTON(giftizBtn)->clicked) {
                    game->gameThreadContext->communicationAPI->giftizButtonClicked();
                }
                game->ignoreClick = BUTTON(giftizBtn)->mouseOver;
            }
            RENDERING(giftizBtn)->color = BUTTON(giftizBtn)->mouseOver ? Color("gray") : Color();

            // Handle help button
            if (!game->ignoreClick) {
                if (BUTTON(helpBtn)->clicked) {
                    return Scene::Tutorial;
                }
                game->ignoreClick = BUTTON(helpBtn)->mouseOver;
            }
            RENDERING(helpBtn)->color = BUTTON(helpBtn)->mouseOver ? Color("gray") : Color();

            // Start game ? (tutorial if no game done)
            if (theTouchInputManager.isTouched(0) && theTouchInputManager.wasTouched(0) && !game->ignoreClick) {
                int current = ObjectSerializer<int>::string2object(game->gameThreadContext->storageAPI->getOption("gameCount"));
                game->gameThreadContext->storageAPI->setOption("gameCount", ObjectSerializer<int>::object2string(current + 1), "0");

                if (current == 0) {
                    return Scene::Tutorial;
                } else {
                    return Scene::Ad;
                }
            }
            return Scene::Menu;
        }


///----------------------------------------------------------------------------//
///--------------------- EXIT SECTION -----------------------------------------//
///----------------------------------------------------------------------------//
        void onPreExit(Scene::Enum ) {
            // stop menu music
            MUSIC(title)->control = MusicControl::Stop;

            // disable button interaction
            BUTTON(goToSocialCenterBtn)->enabled = false;
            BUTTON(giftizBtn)->enabled = false;
            BUTTON(helpBtn)->enabled = false;

            // activate animation
            ADSR(titleGroup)->active = ADSR(subtitle)->active = false;
        }

        bool updatePreExit(Scene::Enum, float) {
            updateTitleSubTitle(titleGroup, subtitle);
            const ADSRComponent* adsr = ADSR(titleGroup);
            float progress = (adsr->value - adsr->attackValue) /
                    (adsr->idleValue - adsr->attackValue);

            RENDERING(goToSocialCenterBtn)->color.a = RENDERING(giftizBtn)->color.a = RENDERING(helpBtn)->color.a = 1 - progress;
            // check if animation is finished
            return (adsr->value >= adsr->idleValue);
        }

        void onExit(Scene::Enum) {
            RENDERING(goToSocialCenterBtn)->show = RENDERING(giftizBtn)->show = RENDERING(helpBtn)->show = false;
        }
};

namespace Scene {
    StateHandler<Scene::Enum>* CreateMenuSceneHandler(RecursiveRunnerGame* game) {
        return new MenuScene(game);
    }
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

static void updateTitleSubTitle(Entity titleGroup, Entity subtitle) {
    TRANSFORM(titleGroup)->position.y = ADSR(titleGroup)->value;
    TRANSFORM(subtitle)->position.y = ADSR(subtitle)->value;
}
