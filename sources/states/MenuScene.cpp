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
static void startMenuMusic(Entity title) {
    MUSIC(title)->music = theMusicSystem.loadMusicFile("sounds/intro-menu.ogg");
    MUSIC(title)->loopNext = theMusicSystem.loadMusicFile("sounds/boucle-menu.ogg");
    MUSIC(title)->loopAt = 4.54;
    MUSIC(title)->volume = 1;
    MUSIC(title)->fadeOut = 2;
    MUSIC(title)->fadeIn = 1;
    MUSIC(title)->control = MusicControl::Play;
}

class MenuScene : public StateHandler<Scene::Enum> {
    RecursiveRunnerGame* game;
    Entity titleGroup, title, subtitle, subtitleText, helpBtn;

    public:
        MenuScene(RecursiveRunnerGame* game) : StateHandler<Scene::Enum>() {
            this->game = game;
        }


        void setup() {
            titleGroup  = theEntityManager.CreateEntity("menu/title_group",
                EntityType::Persistent, theEntityManager.entityTemplateLibrary.load("menu/title_group"));
            ADSR(titleGroup)->idleValue = PlacementHelper::ScreenSize.y + PlacementHelper::GimpYToScreen(400);
            ADSR(titleGroup)->sustainValue = game->baseLine + PlacementHelper::ScreenSize.y
                - PlacementHelper::GimpSizeToScreen(theRenderingSystem.getTextureSize("titre")).y * 0.5
                + PlacementHelper::GimpHeightToScreen(10);
            ADSR(titleGroup)->attackValue = ADSR(titleGroup)->sustainValue - PlacementHelper::GimpHeightToScreen(5);
            TRANSFORM(titleGroup)->position = glm::vec2(game->leftMostCameraPos.x + TRANSFORM(titleGroup)->size.x * 0.5, ADSR(titleGroup)->idleValue);

            title = theEntityManager.CreateEntity("menu/title",
                EntityType::Persistent, theEntityManager.entityTemplateLibrary.load("menu/title"));

            subtitle = theEntityManager.CreateEntity("menu/subtitle",
                EntityType::Persistent, theEntityManager.entityTemplateLibrary.load("menu/subtitle"));

            subtitleText = theEntityManager.CreateEntity("menu/subtitle_text",
                EntityType::Persistent, theEntityManager.entityTemplateLibrary.load("menu/subtitle_text"));
            TEXT(subtitleText)->text = game->gameThreadContext->localizeAPI->text("tap_screen_to_start");
            TEXT(subtitleText)->charHeight *= 1.5;

            helpBtn = theEntityManager.CreateEntity("menu/help_button",
                EntityType::Persistent, theEntityManager.entityTemplateLibrary.load("menu/button"));
            ANCHOR(helpBtn)->parent = game->muteBtn;
            ANCHOR(helpBtn)->position = glm::vec2(0, -(TRANSFORM(helpBtn)->size.y * 0.5 + game->buttonSpacing.V));
            ANCHOR(helpBtn)->z = 0;
            RENDERING(helpBtn)->texture = theRenderingSystem.loadTextureFile("aide");
        }


        ///----------------------------------------------------------------------------//
        ///--------------------- ENTER SECTION ----------------------------------------//
        ///----------------------------------------------------------------------------//
        void onPreEnter(Scene::Enum from) {
            const auto temp = theAutoDestroySystem.RetrieveAllEntityWithComponent();
            std::for_each(temp.begin(), temp.end(), deleteEntityFunctor);

            // activate animation
            ADSR(titleGroup)->active = ADSR(subtitle)->active = true;

            // Restore camera position
            game->setupCamera(CameraMode::Menu);

            // save score if any
            const auto& players = thePlayerSystem.RetrieveAllEntityWithComponent();
            if (!players.empty() && from == Scene::Game) {
                TEXT(subtitleText)->text = ObjectSerializer<int>::object2string(PLAYER(players.front())->points)
                + " " + game->gameThreadContext->localizeAPI->text("points") + " - " + game->gameThreadContext->localizeAPI->text("tap_screen_to_restart");

                //save the score in DB
                ScoreStorageProxy ssp;
                ssp.setValue("points", ObjectSerializer<int>::object2string(PLAYER(players.front())->points), true); // ask to create a new score
                ssp.setValue("coins", ObjectSerializer<int>::object2string(PLAYER(players.front())->coins), false);
                ssp.setValue("name", "rzehtrtyBg", false);
                game->gameThreadContext->storageAPI->saveEntries(&ssp);

                game->updateBestScore();

                game->gameThreadContext->gameCenterAPI->submitScore(0, ssp.getValue("points"));
            }
            // start music if not muted
            if (!theMusicSystem.isMuted() && MUSIC(title)->control == MusicControl::Stop) {
                startMenuMusic(title);
            }
            // unhide UI
            RENDERING(helpBtn)->show = true;
            RENDERING(helpBtn)->color = Color(1,1,1,0);

            game->gamecenterAPIHelper.displayUI();
        }

        bool updatePreEnter(Scene::Enum, float) {
            updateTitleSubTitle(titleGroup, subtitle);
            // check if adsr is complete
            ADSRComponent* adsr = ADSR(titleGroup);
            float progress = (adsr->value - adsr->idleValue) / (adsr->sustainValue - adsr->idleValue);
            RENDERING(helpBtn)->color.a = progress;
            return (adsr->value == adsr->sustainValue);
        }


        void onEnter(Scene::Enum) {
            RecursiveRunnerGame::endGame();
            // enable UI
            BUTTON(helpBtn)->enabled = true;
            RENDERING(helpBtn)->color.a = 1;
        }

#if 0
void backgroundUpdate(float) {
    TRANSFORM(datas->titleGroup)->position.y = ADSR(datas->titleGroup)->value;
    TRANSFORM(datas->subtitle)->position.y = ADSR(datas->subtitle)->value;
}
#endif

        Scene::Enum update(float) {
            // Game center UI - if any button clicked, break the loop
            if (game->gamecenterAPIHelper.updateUI()) {
                return Scene::Menu;
            }


            // Menu music
            if (!theMusicSystem.isMuted()) {
                MusicComponent* music = MUSIC(title);
                music->control = MusicControl::Play;

                if (music->music == InvalidMusicRef) {
                    startMenuMusic(title);
                } else if (music->loopNext == InvalidMusicRef) {
                    music->loopAt = 21.34;
                    music->loopNext = theMusicSystem.loadMusicFile("sounds/boucle-menu.ogg");
                }
            } else if (theMusicSystem.isMuted() != theSoundSystem.mute) {
                // restore music
                if (theTouchInputManager.isTouched(0)) {
                    theMusicSystem.toggleMute(theSoundSystem.mute);
                    game->ignoreClick = true;
                    startMenuMusic(title);
                }
            }

            // Handle help button
            if (!game->ignoreClick) {
                if (BUTTON(helpBtn)->clicked) {
                    return Scene::Tutorial;
                }
                game->ignoreClick = BUTTON(helpBtn)->mouseOver;
            }
            RENDERING(helpBtn)->color = BUTTON(helpBtn)->mouseOver ? Color("gray") : Color();

            // Start game? (tutorial if no game done)
            if (theTouchInputManager.isTouched(0) && theTouchInputManager.wasTouched(0) && !game->ignoreClick) {
                //only if we did not hit left area zone (buttons)
                auto touchPos = theTouchInputManager.getTouchLastPosition(0);
                if (touchPos.x >= TRANSFORM(game->muteBtn)->position.x + TRANSFORM(game->muteBtn)->size.x * BUTTON(game->muteBtn)->overSize * 0.5) {

#if SAC_EMSCRIPTEN
                    return Scene::Game;
#else
                    int current = ObjectSerializer<int>::string2object(game->gameThreadContext->storageAPI->getOption("gameCount"));
                    game->gameThreadContext->storageAPI->setOption("gameCount", ObjectSerializer<int>::object2string(current + 1), "0");

                    if (current == 0) {
                        return Scene::Tutorial;
                    } else {
                        return Scene::Game;
                    }
#endif
                }
            }
#if SAC_BENCHMARK_MODE
            return Scene::Game;
#endif
            return Scene::Menu;
        }


///----------------------------------------------------------------------------//
///--------------------- EXIT SECTION -----------------------------------------//
///----------------------------------------------------------------------------//
        void onPreExit(Scene::Enum ) {
            // stop menu music
            MUSIC(title)->control = MusicControl::Stop;

            // disable button interaction
            BUTTON(helpBtn)->enabled = false;

            game->gamecenterAPIHelper.hideUI();

            // activate animation
            ADSR(titleGroup)->active = ADSR(subtitle)->active = false;
        }

        bool updatePreExit(Scene::Enum, float) {
            updateTitleSubTitle(titleGroup, subtitle);
            const ADSRComponent* adsr = ADSR(titleGroup);
            float progress = (adsr->value - adsr->attackValue) /
                    (adsr->idleValue - adsr->attackValue);

            RENDERING(helpBtn)->color.a = 1 - progress;
            // check if animation is finished
            return (adsr->value >= adsr->idleValue);
        }

        void onExit(Scene::Enum) {
            RENDERING(helpBtn)->show = false;
        }
};

namespace Scene {
    StateHandler<Scene::Enum>* CreateMenuSceneHandler(RecursiveRunnerGame* game) {
        return new MenuScene(game);
    }
}

static void updateTitleSubTitle(Entity titleGroup, Entity subtitle) {
    TRANSFORM(titleGroup)->position.y = ADSR(titleGroup)->value;
    TRANSFORM(subtitle)->position.y = ADSR(subtitle)->value;
}
