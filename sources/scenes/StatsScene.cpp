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

namespace Image {
    enum Enum {
        Background = 0,
        Runner,
        //Coin,
        Count
    };
}

namespace Button {
    enum Enum {
        Back = 0,
        BestScore,
        LastScore,
        Count
    };
}

/*
namespace Text {
    enum Enum {
        Count
    };
}
*/

class StatsScene : public StateHandler<Scene::Enum> {
    RecursiveRunnerGame* game;

    Entity images[Image::Count];
    Entity buttons[Button::Count];
    std::vector<Entity> bars, texts;

public:
    StatsScene(RecursiveRunnerGame* game) : StateHandler<Scene::Enum>("stats"), game(game) {
    }


    void setup(AssetAPI*) override {
        buttons[Button::Back] = theEntityManager.CreateEntityFromTemplate("menu/about/back_button");
        buttons[Button::BestScore] = theEntityManager.CreateEntityFromTemplate("menu/stats/stat_button");
        TEXT(buttons[Button::BestScore])->text = "Best score";

        buttons[Button::LastScore] = theEntityManager.CreateEntityFromTemplate("menu/stats/stat_button");
        TEXT(buttons[Button::LastScore])->text = "Last score";

        images[Image::Background] = theEntityManager.CreateEntityFromTemplate("menu/about/background");
        images[Image::Runner] = theEntityManager.CreateEntityFromTemplate("menu/stats/runner");
        ANCHOR(buttons[Button::Back])->parent = game->muteBtn;
        TRANSFORM(buttons[Button::Back])->position.y += game->baseLine + TRANSFORM(game->cameraEntity)->size.y * 0.5;
    }


    #define ___COLOR(n) Color(((0x##n >> 16) & 0xff) / 255.0, ((0x##n >> 8) & 0xff) / 255.0, ((0x##n >> 0) & 0xff) / 255.0, 0.8)
    const Color colors[10] = {
        ___COLOR(c30101),
        ___COLOR(ea6c06),
        ___COLOR(f4cf00),
        ___COLOR(8ec301),
        ___COLOR(01c373),
        ___COLOR(12bbd9),
        ___COLOR(1e49d7),
        ___COLOR(5d00bd),
        ___COLOR(a910db),
        ___COLOR(dc52b0),
    };
    #undef ___COLOR

    template<class T>
    void addBarWithLegend(const char* text, int color, float xOffset, int offset, const char* format, T maxValue, T minValue, T minDisplay) {
        bool addGlobalLegend = true;
        char tmp[64];
        /* score bar */
        for (int i=0; i<10; i++) {
            const T value = *((T*) ((uint8_t*)&statisticsToDisplay->runner[i] + offset));
            if (value < minValue) continue;

            /* legend */



            Entity bar = theEntityManager.CreateEntityFromTemplate("menu/stats/bar");
            TRANSFORM(bar)->size.y = glm::lerp(0.1f, 8.0f, value / (float)maxValue);
            ANCHOR(bar)->anchor.y = - TRANSFORM(bar)->size.y * 0.5;
            ANCHOR(bar)->position.x = xOffset;

            ANCHOR(bar)->parent = texts[i];
            RENDERING(bar)->color = colors[color];

            bars.push_back(bar);

            if (value >= minDisplay) {
                Entity legend = theEntityManager.CreateEntityFromTemplate("menu/stats/bar_legend");
                snprintf(tmp, 64, format, value);
                TEXT(legend)->text = tmp;
                TEXT(legend)->positioning = 0.5;
                ANCHOR(legend)->parent = bar;
                texts.push_back(legend);
            }

            if (1)/* if (addGlobalLegend && value < maxValue)*/ {
                Entity t = theEntityManager.CreateEntityFromTemplate("menu/stats/bar_legend");
                TEXT(t)->text = text;
                TEXT(t)->color = colors[color];
                ANCHOR(t)->parent = bar;
                ANCHOR(t)->position.y = 0.5 + TRANSFORM(bar)->size.y * 0.5;
                texts.push_back(t);
                addGlobalLegend = false;
            }
        }
    }

    Statistics* statisticsToDisplay;
    void onPreEnter(Scene::Enum) {
        if (game->lastGameStatistics->score > 0) {
            RENDERING(buttons[Button::LastScore])->texture = theRenderingSystem.loadTextureFile("stats_btn_back_active");
            statisticsToDisplay = game->lastGameStatistics;
        } else {
            RENDERING(buttons[Button::BestScore])->texture = theRenderingSystem.loadTextureFile("stats_btn_back_active");
            statisticsToDisplay = game->bestGameStatistics;
        }
        TRANSFORM(buttons[Button::LastScore])->position.x = TRANSFORM(game->camera)->position.x + TRANSFORM(buttons[Button::LastScore])->size.x;
        TRANSFORM(buttons[Button::BestScore])->position.x = TRANSFORM(game->camera)->position.x - TRANSFORM(buttons[Button::BestScore])->size.x;
    }

    ///----------------------------------------------------------------------------//
    ///--------------------- ENTER SECTION ----------------------------------------//
    ///----------------------------------------------------------------------------//
    void onEnter(Scene::Enum) {
        TEXT(buttons[Button::LastScore])->show = 1;
        TEXT(buttons[Button::BestScore])->show = 1;

        int maxPointScored = 1;
        int maxJumps = 1; /* avoid divide by 0 */
        int maxKilledScore = 1; /* avoid divide by 0 */
        for (int i=0; i<10; i++) {
            maxPointScored = glm::max(maxPointScored, statisticsToDisplay->runner[i].pointScored);
            maxJumps = glm::max(maxJumps, statisticsToDisplay->runner[i].jumps);
            maxKilledScore = glm::max(maxKilledScore, statisticsToDisplay->runner[i].killed);
        }

        char tmp[64];
        /* x-axis legend */
        for (int i=0; i<10; i++) {
            texts.push_back(theEntityManager.CreateEntityFromTemplate("menu/stats/runner_text"));
            TRANSFORM(texts[i])->position.x += 1.6 * (i % 10);

            snprintf(tmp, 20, "%d", i+1);
            TEXT(texts[i])->text = tmp;
        }

        /* score bar */
        addBarWithLegend<int>("Points", 4, -0.45, OFFSET(pointScored, statisticsToDisplay->runner[0]), "%d", maxPointScored, 0, maxPointScored / 20);
        /* score bar */
        addBarWithLegend<int>("Coins", 7, -.15, OFFSET(coinsCollected, statisticsToDisplay->runner[0]), "%d", 20, 0, 1);
        /* killed bar */
        /* addBarWithLegend<int>("%d kills", 1, -0.15, OFFSET(killed, statisticsToDisplay->runner[0]), "%d", maxKilledScore, 0, 1); */
        /* time bar */
        addBarWithLegend<float>("Seconds", 2, 0.15, OFFSET(lifetime, statisticsToDisplay->runner[0]), "%.0f", 91, 0, 1);
        /* jump bar */
        addBarWithLegend<int>("Jumps", 3, 0.45, OFFSET(jumps, statisticsToDisplay->runner[0]), "%d", maxJumps, 0, 1);

        for (auto t: texts) {
            TEXT(t)->show = true;
        }
        for (int i=0; i<Button::Count; i++) {
            RENDERING(buttons[i])->show =
                BUTTON(buttons[i])->enabled = true;
        }
        for (int i=0; i<Image::Count; i++) {
            RENDERING(images[i])->show = true;
        }
        for (auto b: bars) {
            RENDERING(b)->show = true;
        }
    }

    ///----------------------------------------------------------------------------//
    ///--------------------- UPDATE SECTION ---------------------------------------//
    ///----------------------------------------------------------------------------//

    Scene::Enum update(float) override {
        RENDERING(buttons[Button::Back])->color = BUTTON(buttons[Button::Back])->mouseOver ? Color("gray") : Color();

        if (BUTTON(buttons[Button::BestScore])->clicked) {
            onPreExit(Scene::Stats);
            statisticsToDisplay = game->bestGameStatistics;

            RENDERING(buttons[Button::BestScore])->texture = theRenderingSystem.loadTextureFile("stats_btn_back_active");
            RENDERING(buttons[Button::LastScore])->texture = theRenderingSystem.loadTextureFile("stats_btn_back");
            onEnter(Scene::Stats);
        }
        if (BUTTON(buttons[Button::LastScore])->clicked) {
            onPreExit(Scene::Stats);
            statisticsToDisplay = game->lastGameStatistics;
            RENDERING(buttons[Button::LastScore])->texture = theRenderingSystem.loadTextureFile("stats_btn_back_active");
            RENDERING(buttons[Button::BestScore])->texture = theRenderingSystem.loadTextureFile("stats_btn_back");
            onEnter(Scene::Stats);
        }

        if (BUTTON(buttons[Button::Back])->clicked) {
            return Scene::Menu;
        }

        return Scene::Stats;
    }


///----------------------------------------------------------------------------//
///--------------------- EXIT SECTION -----------------------------------------//
///----------------------------------------------------------------------------//
    void onPreExit(Scene::Enum ) override {
        for (int i=0; i<Button::Count; i++) {
            RENDERING(buttons[i])->show =
                BUTTON(buttons[i])->enabled = false;
        }
        for (int i=0; i<Image::Count; i++) {
            RENDERING(images[i])->show = false;
        }

        for (auto b: bars) {
            theEntityManager.DeleteEntity(b);
        }
        bars.clear();

        for (auto b: texts) {
            theEntityManager.DeleteEntity(b);
        }
        texts.clear();
        TEXT(buttons[Button::LastScore])->show = 0;
        TEXT(buttons[Button::BestScore])->show = 0;
    }


    void onExit(Scene::Enum) override {

    }
};

namespace Scene {
    StateHandler<Scene::Enum>* CreateStatsSceneHandler(RecursiveRunnerGame* game) {
        return new StatsScene(game);
    }
}
