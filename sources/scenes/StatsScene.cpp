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
    std::vector<Entity> bars, texts;

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


    Entity createText(const char* text, const glm::vec2& pos) {
        Entity t = theEntityManager.CreateEntityFromTemplate("menu/stats/bar_legend");
        TEXT(t)->text = text;
        ANCHOR(t)->parent = images[Image::Background];
        ANCHOR(t)->position = pos;
        texts.push_back(t);
        return t;
    }

    template<class T>
    Entity createTextWithValue(T value, const char* format, const glm::vec2& pos) {
        char tmp[128];
        sprintf(tmp, format, value);
        return createText(tmp, pos);
    }


    int runnerPointMax(const Statistics* s) {
        int m = 0;
        for (int i=0; i<10; i++) m = glm::max(m, s->runner[i].pointScored);
        return m;
    }
    int runnerCoinAverage(const Statistics* s) {
        int sum = 0;
        for (int i=0; i<10; i++) sum += s->runner[i].coinsCollected;
        return sum * 0.1;
    }
    int runnerLifetimeAverage(const Statistics* s) {
        float sum = 0;
        for (int i=0; i<10; i++) sum += s->runner[i].lifetime;
        return (int) (sum * 0.1);
    }
    int runnerJumpsAverage(const Statistics* s) {
        float sum = 0;
        for (int i=0; i<10; i++) sum += s->runner[i].jumps;
        return (int) (sum * 0.1);
    }

    ///----------------------------------------------------------------------------//
    ///--------------------- ENTER SECTION ----------------------------------------//
    ///----------------------------------------------------------------------------//
    void onEnter(Scene::Enum) {
        const glm::vec2& area = TRANSFORM(images[Image::Background])->size;
        const glm::vec2 spacing = area * 0.05f;

        int rows = 6;
        int columns = 4;
        glm::vec2 cellSize = (area - 2.0f * spacing) * glm::vec2(1.0f / columns, 1.0f / rows);

        glm::vec2 cell0 = area * - 0.5f + spacing + cellSize * 0.5f;

        #define P(x, y) (cell0 + cellSize * glm::vec2(y, rows - x - 1))

        #define Q(__x, __y) (cell0 + cellSize * glm::vec2(__y, rows - __x - 1) +  glm::vec2(cellSize.x * 0.35f, 0.0f))

        TRANSFORM(images[Image::Runner])->position = TRANSFORM(images[Image::Background])->position + P(0, 0);
        /* Columns header */
        createText("Last game", P(0, 1));
        TEXT(texts.back())->positioning = 0.5;
        createText("Session best", P(0, 2));
        TEXT(texts.back())->positioning = 0.5;
        createText("All time best", P(0, 3));
        TEXT(texts.back())->positioning = 0.5;

        /* Total points */
        createText("Points", P(1, 0));
        TEXT(texts.back())->positioning = 0.5;
        createTextWithValue<int>(game->statistics.lastGame->score, "%d", Q(1, 1));
        createTextWithValue<int>(game->statistics.sessionBest->score, "%d", Q(1, 2));
        createTextWithValue<int>(game->statistics.allTimeBest->score, "%d", Q(1, 3));

        /* Points per runner */
        createText("Max runner point", P(2, 0));
        TEXT(texts.back())->positioning = 0.5;
        createTextWithValue<int>(runnerPointMax(game->statistics.lastGame), "%d", Q(2, 1));
        createTextWithValue<int>(runnerPointMax(game->statistics.sessionBest), "%d", Q(2, 2));
        createTextWithValue<int>(runnerPointMax(game->statistics.allTimeBest), "%d", Q(2, 3));

        /* Coins per runner */
        createText("Average switches", P(3, 0));
        TEXT(texts.back())->positioning = 0.5;
        createTextWithValue<int>(runnerCoinAverage(game->statistics.lastGame), "%d", Q(3, 1));
        createTextWithValue<int>(runnerCoinAverage(game->statistics.sessionBest), "%d", Q(3, 2));
        createTextWithValue<int>(runnerCoinAverage(game->statistics.allTimeBest), "%d", Q(3, 3));

        /* Lifetime per runner */
        createText("Average lifetime", P(4, 0));
        TEXT(texts.back())->positioning = 0.5;
        createTextWithValue<int>(runnerLifetimeAverage(game->statistics.lastGame), "%d", Q(4, 1));
        createTextWithValue<int>(runnerLifetimeAverage(game->statistics.sessionBest), "%d", Q(4, 2));
        createTextWithValue<int>(runnerLifetimeAverage(game->statistics.allTimeBest), "%d", Q(4, 3));

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
    }


    void onExit(Scene::Enum) override {

    }
};

namespace Scene {
    StateHandler<Scene::Enum>* CreateStatsSceneHandler(RecursiveRunnerGame* game) {
        return new StatsScene(game);
    }
}
