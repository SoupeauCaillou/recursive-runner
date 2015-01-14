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
#include <mutex>
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

namespace Button {
    enum Enum {
        Help = 0,
        About,
        // Exit,
        Count
    };
}

class MenuScene : public StateHandler<Scene::Enum> {
    RecursiveRunnerGame* game;
    Entity titleGroup, title, subtitle, subtitleText;
    Entity buttons[Button::Count];
    std::mutex m;
    int weeklyRank;

    public:
        MenuScene(RecursiveRunnerGame* game) : StateHandler<Scene::Enum>("menu") {
            this->game = game;
        }


        void setup(AssetAPI*) override {
            titleGroup  = theEntityManager.CreateEntity(HASH("menu/title_group", 0x5affe5af),
                EntityType::Persistent, theEntityManager.entityTemplateLibrary.load("menu/title_group"));
            ADSR(titleGroup)->idleValue = PlacementHelper::GimpYToScreen(400);
            ADSR(titleGroup)->sustainValue = game->baseLine
                - PlacementHelper::GimpSizeToScreen(theRenderingSystem.getTextureSize("titre")).y * 0.5
                + PlacementHelper::GimpHeightToScreen(30);
            ADSR(titleGroup)->attackValue = ADSR(titleGroup)->sustainValue - PlacementHelper::GimpHeightToScreen(5);
            TRANSFORM(titleGroup)->position = glm::vec2(game->leftMostCameraPos.x + TRANSFORM(titleGroup)->size.x * 0.5, ADSR(titleGroup)->idleValue);

            title = theEntityManager.CreateEntityFromTemplate("menu/title");

            subtitle = theEntityManager.CreateEntityFromTemplate("menu/subtitle");

            subtitleText = theEntityManager.CreateEntityFromTemplate("menu/subtitle_text");
            TEXT(subtitleText)->text = game->gameThreadContext->localizeAPI->text("Tap screen to start");
            TEXT(subtitleText)->charHeight *= 1.5;

            buttons[Button::Help] = theEntityManager.CreateEntity(HASH("menu/help_button", 0xf0532382),
                EntityType::Persistent, theEntityManager.entityTemplateLibrary.load("menu/button"));
            ANCHOR(buttons[Button::Help])->parent = game->muteBtn;
            ANCHOR(buttons[Button::Help])->position = glm::vec2(0, -(TRANSFORM(buttons[Button::Help])->size.y * 0.5 + game->buttonSpacing.V));
            ANCHOR(buttons[Button::Help])->z = 0;
            RENDERING(buttons[Button::Help])->texture = HASH("aide", 0xc3acc704);


            buttons[Button::About] = theEntityManager.CreateEntityFromTemplate("menu/about_btn");
            ANCHOR(buttons[Button::About])->parent = game->muteBtn;
            // buttons[Button::Exit] = theEntityManager.CreateEntityFromTemplate("menu/exit_btn");
            // ANCHOR(buttons[Button::Exit])->parent = game->muteBtn;
            // sigh
            TRANSFORM(buttons[Button::About])->position.y += game->baseLine + TRANSFORM(game->cameraEntity)->size.y * 0.5;

            weeklyRank = -1;
        }


        ///----------------------------------------------------------------------------//
        ///--------------------- ENTER SECTION ----------------------------------------//
        ///----------------------------------------------------------------------------//
        void onPreEnter(Scene::Enum from) {
            // activate animation
            ADSR(titleGroup)->active = ADSR(subtitle)->active = true;

            // Restore camera position
            game->setupCamera(CameraMode::Menu);

            // save score if any
            const auto& players = thePlayerSystem.RetrieveAllEntityWithComponent();
            if (!players.empty() && from == Scene::Game) {
                TEXT(subtitleText)->text = ObjectSerializer<int>::object2string(PLAYER(players.front())->points)
                + " " + game->gameThreadContext->localizeAPI->text("points") + " - " + game->gameThreadContext->localizeAPI->text("tap screen to restart");

                //save the score in DB
                ScoreStorageProxy ssp;
                ssp.setValue("points", ObjectSerializer<int>::object2string(PLAYER(players.front())->points), true); // ask to create a new score
                ssp.setValue("coins", ObjectSerializer<int>::object2string(PLAYER(players.front())->coins), false);
                ssp.setValue("name", "rzehtrtyBg", false);
                game->gameThreadContext->storageAPI->saveEntries(&ssp);

                game->updateBestScore();

                #if SAC_USE_PROPRIETARY_PLUGINS
                    // Submit score to generic leaderboard
                    game->gameThreadContext->gameCenterAPI->submitScore(0, ssp.getValue("points"));
                    if (game->level == Level::Level2) {
                        // Submit score to daily leaderboard
                        // time_t t = time(0);
                        // struct tm * timeinfo = localtime (&t);
                        game->gameThreadContext->gameCenterAPI->submitScore(1 /*+ tm->tm_mday*/, ssp.getValue("points"));
                        // retrieve weekly rank
                        game->gameThreadContext->gameCenterAPI->getWeeklyRank(1 /*+ tm->tm_mday*/, [this] (int rank) -> void {
                                std::unique_lock<std::mutex> l(m);
                                weeklyRank = rank;
                                LOGI(__(rank));
                                //__android_log_print(ANDROID_LOG_ERROR, "sac", "RANK: %d", rank);
                            }
                        );
                    }
                #endif
            }
            // start music if not muted
            if (!theMusicSystem.isMuted() && MUSIC(title)->control == MusicControl::Stop) {
                startMenuMusic(title);
            }
            // unhide UI
            for (int i=0; i<(int)Button::Count; i++) {
                RENDERING(buttons[i])->show = true;
                RENDERING(buttons[i])->color = Color(1,1,1,0);
            }
            RENDERING(game->muteBtn)->show = true;
            RENDERING(game->muteBtn)->color = Color(1,1,1,0);

        }

        bool updatePreEnter(Scene::Enum, float) {
            updateTitleSubTitle(titleGroup, subtitle);
            // check if adsr is complete
            ADSRComponent* adsr = ADSR(titleGroup);
            float progress = (adsr->value - adsr->idleValue) / (adsr->sustainValue - adsr->idleValue);

            for (int i=0; i<(int)Button::Count; i++) {
                RENDERING(buttons[i])->color.a = progress;
            }
            RENDERING(game->muteBtn)->color.a = progress;

            #if SAC_USE_PROPRIETARY_PLUGINS
                game->gamecenterAPIHelper.displayUI(progress);
            #endif

            return (adsr->value == adsr->sustainValue);
        }


        void onEnter(Scene::Enum) {
            game->endGame(game->statistics.lastGame);
            if (game->statisticsAvailable()) {
                BUTTON(game->statman)->enabled = true;
                RENDERING(game->statman)->texture = theRenderingSystem.loadTextureFile("statman_panneau");
            } else {
                BUTTON(game->statman)->enabled = false;
                RENDERING(game->statman)->texture = theRenderingSystem.loadTextureFile("statman_gauche");
            }

            // enable UI
            for (int i=0; i<(int)Button::Count; i++) {
                BUTTON(buttons[i])->enabled = true;
                RENDERING(buttons[i])->color.a = 1;
            }
            BUTTON(game->muteBtn)->enabled = true;
            RENDERING(game->muteBtn)->color.a = 1;
            #if SAC_USE_PROPRIETARY_PLUGINS
                game->gamecenterAPIHelper.displayUI();
            #endif
        }

#if 0
void backgroundUpdate(float) {
    TRANSFORM(datas->titleGroup)->position.y = ADSR(datas->titleGroup)->value;
    TRANSFORM(datas->subtitle)->position.y = ADSR(datas->subtitle)->value;
}
#endif

        Scene::Enum update(float) {
#if SAC_BENCHMARK_MODE
            return Scene::Game;
#endif

            updateTitleSubTitle(titleGroup, subtitle);

            #if SAC_USE_PROPRIETARY_PLUGINS
                // Game center UI - if any button clicked, break the loop
                if (game->gamecenterAPIHelper.updateUI()) {
                    return Scene::Menu;
                }
            #endif

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
                if (BUTTON(buttons[Button::Help])->clicked) {
                    return Scene::Tutorial;
                }
                if (BUTTON(buttons[Button::About])->clicked) {
                   return Scene::About;
                }
                if (BUTTON(game->statman)->clicked) {
                    game->ignoreClick = true;
                   return Scene::Stats;
                }

                for (int i=0; i<Button::Count; i++) {
                    game->ignoreClick |= BUTTON(buttons[i])->mouseOver;
                }
                game->ignoreClick |= BUTTON(game->statman)->mouseOver;
            }
            RENDERING(buttons[Button::Help])->color = BUTTON(buttons[Button::Help])->mouseOver ? Color("gray") : Color();
            RENDERING(buttons[Button::About])->color = BUTTON(buttons[Button::About])->mouseOver ? Color("gray") : Color();

            // Start game? (tutorial if no game done)
            if (!theTouchInputManager.isTouched(0) && theTouchInputManager.wasTouched(0) && !game->ignoreClick) {
                //only if we did not hit left area zone (buttons)
                auto touchPos = theTouchInputManager.getTouchLastPosition(0);
                if (touchPos.x >= (TRANSFORM(game->muteBtn)->position.x + TRANSFORM(game->muteBtn)->size.x * BUTTON(game->muteBtn)->overSize * 0.5) &&
                touchPos.y < TRANSFORM(subtitleText)->position.y) {

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

            return Scene::Menu;
        }


///----------------------------------------------------------------------------//
///--------------------- EXIT SECTION -----------------------------------------//
///----------------------------------------------------------------------------//
        void onPreExit(Scene::Enum nextScene ) {
            // stop menu music
            MUSIC(title)->control = MusicControl::Stop;

            // disable button interaction
            for (int i=0; i<(int)Button::Count; i++) {
                BUTTON(buttons[i])->enabled = false;
            }
            if(nextScene == Scene::About || nextScene == Scene::Stats)
                BUTTON(game->muteBtn)->enabled = false;

            // activate animation
            ADSR(titleGroup)->active = ADSR(subtitle)->active = false;
        }

        bool updatePreExit(Scene::Enum nextScene, float) {
            updateTitleSubTitle(titleGroup, subtitle);
            const ADSRComponent* adsr = ADSR(titleGroup);
            float progress = (adsr->value - adsr->attackValue) /
                    (adsr->idleValue - adsr->attackValue);

            for (int i=0; i<(int)Button::Count; i++) {
                RENDERING(buttons[i])->color.a = 1 - progress;
            }
            if(nextScene == Scene::About || nextScene == Scene::Stats)
                RENDERING(game->muteBtn)->color.a = 1 - progress;

            #if SAC_USE_PROPRIETARY_PLUGINS
                game->gamecenterAPIHelper.hideUI(1 - progress);
            #endif

            // check if animation is finished
            return (adsr->value >= adsr->idleValue);
        }

        void onExit(Scene::Enum nextScene) {
            for (int i=0; i<(int)Button::Count; i++) {
                RENDERING(buttons[i])->show = false;
            }
            if(nextScene == Scene::About || nextScene == Scene::Stats)
                RENDERING(game->muteBtn)->show = false;

            #if SAC_USE_PROPRIETARY_PLUGINS
                game->gamecenterAPIHelper.hideUI();
            #endif
        }
};

namespace Scene {
    StateHandler<Scene::Enum>* CreateMenuSceneHandler(RecursiveRunnerGame* game) {
        return new MenuScene(game);
    }
}

static void updateTitleSubTitle(Entity titleGroup, Entity subtitle) {
    TRANSFORM(titleGroup)->position.y = + PlacementHelper::ScreenSize.y + ADSR(titleGroup)->value;
    TRANSFORM(subtitle)->position.y = PlacementHelper::ScreenSize.y + ADSR(subtitle)->value;
}
