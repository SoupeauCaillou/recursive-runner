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
        Count
    };
}


namespace Text {
    enum Enum {
        /*AllTimeBest = 0,
        SessionBest,
        LastGame,*/
        Count
    };
}

class StatsScene : public StateHandler<Scene::Enum> {
    RecursiveRunnerGame* game;

    Entity images[Image::Count];
    Entity buttons[Button::Count];
    std::vector<Entity> bars, texts, legends;

public:
    StatsScene(RecursiveRunnerGame* game) : StateHandler<Scene::Enum>("stats"), game(game) {
    }


    void setup(AssetAPI*) override {
        buttons[Button::Back] = theEntityManager.CreateEntityFromTemplate("menu/about/back_button");

        images[Image::Background] = theEntityManager.CreateEntityFromTemplate("menu/about/background");
        images[Image::Runner] = theEntityManager.CreateEntityFromTemplate("menu/stats/runner");
        ANCHOR(buttons[Button::Back])->parent = game->muteBtn;
        TRANSFORM(buttons[Button::Back])->position.y += game->baseLine + TRANSFORM(game->cameraEntity)->size.y * 0.5;
    }


    Entity createText(const char* text, const glm::vec2& pos, const Color& c) {
        Entity t = theEntityManager.CreateEntityFromTemplate("menu/stats/bar_legend");
        TEXT(t)->text = text;
        TEXT(t)->color = c;
        ANCHOR(t)->parent = images[Image::Background];
        ANCHOR(t)->position = pos;
        texts.push_back(t);
        return t;
    }

    template<class T>
    Entity createTextWithValue(T value, const char* format, const glm::vec2& pos, const Color& c) {
        char tmp[128];
        sprintf(tmp, format, value);
        return createText(tmp, pos, c);
    }

    #define ___COLOR(n) Color(((0x##n >> 16) & 0xff) / 255.0, ((0x##n >> 8) & 0xff) / 255.0, ((0x##n >> 0) & 0xff) / 255.0, 1)
    const Color colors[10] = {
        ___COLOR(3a4246),
        ___COLOR(575362),
        ___COLOR(5e696f),
        ___COLOR(8ec301),
        ___COLOR(01c373),
        ___COLOR(12bbd9),
        ___COLOR(1e49d7),
        ___COLOR(5d00bd),
        ___COLOR(a910db),
        ___COLOR(dc52b0),
    };


#if 0
     template<class T>
    void addBarWithLegend(float maxBarHeight, const char* text, int color, float xOffset, int offset, const char* format, T maxValue, T minValue, T minDisplay) {

        char tmp[64];
        /* score bar */
        for (int i=0; i<10; i++) {
            const T value = *((T*) ((uint8_t*)&statisticsToDisplay->runner[i] + offset));
            if (value < minValue) continue;

            /* legend */
            Entity bar = theEntityManager.CreateEntityFromTemplate("menu/stats/bar");
            TRANSFORM(bar)->size.y = glm::lerp(0.1f, maxBarHeight, value / (float)maxValue);
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
            }
        }
    }
#endif
    int runnerPointMax(const Statistics* s) {
        int m = 0;
        for (int i=0; i<10; i++) m = glm::max(m, s->runner[i].pointScored);
        return m;
    }
    int runnerPointMin(const Statistics* s) {
        int m = INT_MAX;
        for (int i=0; i<10; i++) m = glm::min(m, s->runner[i].pointScored);
        return m;
    }
    int runnerPointAverage(const Statistics* s) {
        int m = 0;
        for (int i=0; i<10; i++) m += s->runner[i].pointScored;
        return m * 0.1;
    }

    float indexToScale(int idx) {
        switch (idx) {
            case 2: return 0.8;
            case 1: return 0.5f;
            default: return 0.2;
        }
    }
    ///----------------------------------------------------------------------------//
    ///--------------------- ENTER SECTION ----------------------------------------//
    ///----------------------------------------------------------------------------//
    void onEnter(Scene::Enum) {
        int maxPointScored = 1;
        for (int i=0; i<10; i++) {
            for (int j=0; j<3; j++) {
                    maxPointScored = glm::max(maxPointScored, game->statistics.s[j]->runner[i].pointScored);
            }
        }
        const glm::vec2& area = TRANSFORM(images[Image::Background])->size;
        const glm::vec2 spacing = area * glm::vec2(0.1f, 0.1f);

        int rows = 6;
        int columns = 3;
        glm::vec2 cellSize = (area - 2.0f * spacing) * glm::vec2(1.0f / columns, 1.0f / rows);

        glm::vec2 cell0 = area * - 0.5f + spacing + cellSize * 0.5f;

        #define P(x, y) (cell0 + cellSize * glm::vec2(y, rows - x - 1))

        #define Q(__x, __y) (cell0 + cellSize * glm::vec2(__y, rows - __x - 1) +  glm::vec2(cellSize.x * 0.35f, 0.0f))

        /* Columns header */
        createTextWithValue(game->statistics.lastGame->score, "Last game\n%d", P(0, 0), colors[2]);
        TEXT(texts.back())->positioning = 0.5;
        TEXT(texts.back())->flags |= TextComponent::MultiLineBit;
        TRANSFORM(texts.back())->size.x = cellSize.x;
        createTextWithValue(game->statistics.sessionBest->score, "Session best\n%d", P(0, 1), colors[1]);
        TEXT(texts.back())->positioning = 0.5;
        TEXT(texts.back())->flags |= TextComponent::MultiLineBit;
        TRANSFORM(texts.back())->size.x = cellSize.x;
        createTextWithValue(game->statistics.allTimeBest->score, "All time best\n%d", P(0, 2), colors[0]);
        TEXT(texts.back())->positioning = 0.5;
        TEXT(texts.back())->flags |= TextComponent::MultiLineBit;
        TRANSFORM(texts.back())->size.x = cellSize.x;
        for (int i=0; i<3; i++) {
            ANCHOR(texts[i])->position.y = area.y * 0.5 - TEXT(texts[i])->charHeight * 1.5;
        }



        float maxScoreTextWidth = 0;
        /* Add y-axis label */
        {
            Entity yaxis = theEntityManager.CreateEntityFromTemplate("menu/stats/bar_legend");
            ANCHOR(yaxis)->position.x = -area.x * 0.5 + spacing.x * 0.5;
            ANCHOR(yaxis)->rotation = glm::pi<float>() * 0.5f;
            ANCHOR(yaxis)->position.y = 0;
            ANCHOR(yaxis)->parent = images[Image::Background];

            TEXT(yaxis)->text = "1000000";
            maxScoreTextWidth = theTextSystem.computeTextComponentWidth(TEXT(yaxis));
            TEXT(yaxis)->text = "Points";
            texts.push_back(yaxis);
        }

        char tmp[64];
        float maxBarHeight = 2 * ANCHOR(texts[0])->position.y - spacing.y - maxScoreTextWidth;
        float width = (area.x - spacing.x * 2.0f) / 10.0f;
        glm::vec2 base = area * -0.5f + spacing + glm::vec2(width * 0.5f, 0.f);
        for (int i=0; i<10; i++) {
            int scores[3], sorted[3];
            for (int j=0; j<3; j++) {
                scores[j] = game->statistics.s[j]->runner[i].pointScored;
                sorted[j] = scores[j];
            }
            std::sort(sorted, sorted + 3, [] (int p, int q) -> bool { return p > q; });

            for (int j=0; j<3; j++) {
                int value = game->statistics.s[j]->runner[i].pointScored;
                int index = j;
                int indexSorted = std::find(sorted, sorted + 3, value) - sorted; // d € [0, 1, 2]
                LOGE_IF(index < 0 || index > 2, "Uh");
                Entity bar = theEntityManager.CreateEntityFromTemplate("menu/stats/bar");
                ANCHOR(bar)->position = base;
                ANCHOR(bar)->parent = images[Image::Background];
                ANCHOR(bar)->z = 0.02 + (2 - index) * 0.02;
                RENDERING(bar)->color = colors[j];

                float height = glm::lerp(0.1f, maxBarHeight, value / (float)maxPointScored);
                TRANSFORM(bar)->size = glm::vec2(width * indexToScale(index), 0.1);
                ANCHOR(bar)->anchor.y = 0; //-TRANSFORM(bar)->size.y * 0.5f;
                ADSR(bar)->idleValue = 0.1;
                ADSR(bar)->attackValue =
                    ADSR(bar)->sustainValue = height;
                ADSR(bar)->attackTiming = 1;
                ADSR(bar)->active = true;

                if (indexSorted == 0) {
                    // add legend
                    Entity legend = theEntityManager.CreateEntityFromTemplate("menu/stats/bar_legend");
                    ANCHOR(legend)->position.y = 0;//TRANSFORM(bar)->size.y * 0.5;
                    ANCHOR(legend)->parent = bar;
                    ANCHOR(legend)->z = 0;
                    ANCHOR(legend)->rotation = glm::pi<float>() * 0.5;

                    snprintf(tmp, 64, " %d", value);
                    TEXT(legend)->text = tmp;
                    TEXT(legend)->positioning = 0;
                    TEXT(legend)->color = colors[j];//___COLOR(827475);

                    // float w = computeTextComponentWidth(TEXT(legend));
                    texts.push_back(legend);
                    legends.push_back(legend);
                }


                bars.push_back(bar);
            }

            {
                Entity t = theEntityManager.CreateEntityFromTemplate("menu/stats/bar_legend");
                ANCHOR(t)->position.x = base.x;
                ANCHOR(t)->position.y = base.y - spacing.y * 0.3;
                ANCHOR(t)->parent = images[Image::Background];

                snprintf(tmp, 20, "%d", i+1);
                TEXT(t)->text = tmp;
                TEXT(t)->positioning = 0.5;
                texts.push_back(t);
            }
            base.x += width;
        }

        TRANSFORM(images[Image::Runner])->position = TRANSFORM(images[Image::Background])->position + base - glm::vec2(spacing.x * 0.25f, 0.0f);


#if 0
        /* Total points */
        createText("Points", P(1, 0));
        TEXT(texts.back())->positioning = 0.5;
        createTextWithValue<int>(game->statistics.lastGame->score, "%d", Q(1, 1));
        createTextWithValue<int>(game->statistics.sessionBest->score, "%d", Q(1, 2));
        createTextWithValue<int>(game->statistics.allTimeBest->score, "%d", Q(1, 3));

        /* Points per runner */
        createText("Max points", P(2, 0));
        TEXT(texts.back())->positioning = 0.5;
        createTextWithValue<int>(runnerPointMax(game->statistics.lastGame), "%d", Q(2, 1));
        createTextWithValue<int>(runnerPointMax(game->statistics.sessionBest), "%d", Q(2, 2));
        createTextWithValue<int>(runnerPointMax(game->statistics.allTimeBest), "%d", Q(2, 3));

        /* Coins per runner */
        createText("Min points", P(3, 0));
        TEXT(texts.back())->positioning = 0.5;
        createTextWithValue<int>(runnerPointMin(game->statistics.lastGame), "%d", Q(3, 1));
        createTextWithValue<int>(runnerPointMin(game->statistics.sessionBest), "%d", Q(3, 2));
        createTextWithValue<int>(runnerPointMin(game->statistics.allTimeBest), "%d", Q(3, 3));

        /* Lifetime per runner */
        createText("Average", P(4, 0));
        TEXT(texts.back())->positioning = 0.5;
        createTextWithValue<int>(runnerPointAverage(game->statistics.lastGame), "%d", Q(4, 1));
        createTextWithValue<int>(runnerPointAverage(game->statistics.sessionBest), "%d", Q(4, 2));
        createTextWithValue<int>(runnerPointAverage(game->statistics.allTimeBest), "%d", Q(4, 3));
#endif
#if 0
        /* Jumps per runner */
        /* Lifetime per runner */
        createText("Average jumps", P(5, 0));
        TEXT(texts.back())->positioning = 0.5;
        createTextWithValue<int>(runnerJumpsAverage(game->statistics.lastGame), "%d", Q(5, 1));
        createTextWithValue<int>(runnerJumpsAverage(game->statistics.sessionBest), "%d", Q(5, 2));
        createTextWithValue<int>(runnerJumpsAverage(game->statistics.allTimeBest), "%d", Q(5, 3));
        /* create backgrounds */
        for (int i=1; i<rows; i+=2) {
            Entity b = theEntityManager.CreateEntityFromTemplate("menu/stats/bar");
            ANCHOR(b)->position.y = P(i, 0).y;
            ANCHOR(b)->parent = images[Image::Background];
            TRANSFORM(b)->size = cellSize * glm::vec2(columns, 1);
            bars.push_back(b);
        }
#endif

        #undef P
        #undef Q

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

        for (auto b: bars) {
            float height = ADSR(b)->value;
            TRANSFORM(b)->size.y = height;
            ANCHOR(b)->anchor.y = -height * 0.5;
        }
        for (auto l: legends) {
            ANCHOR(l)->position.y = TRANSFORM(ANCHOR(l)->parent)->size.y * 0.5;
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
        legends.clear();
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
    }


    void onExit(Scene::Enum) override {

    }
};
#undef ___COLOR
namespace Scene {
    StateHandler<Scene::Enum>* CreateStatsSceneHandler(RecursiveRunnerGame* game) {
        return new StatsScene(game);
    }
}
