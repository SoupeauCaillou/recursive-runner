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

#include "base/PlacementHelper.h"

#include "systems/TransformationSystem.h"
#include "systems/RenderingSystem.h"

#include "../RecursiveRunnerGame.h"

class AdScene : public StateHandler<Scene::Enum> {
    RecursiveRunnerGame* game;
    int gamesb4Ad;
    float lastAdTime;
    float waitAfterAdDisplay;

    public:
        AdScene(RecursiveRunnerGame* game) : StateHandler<Scene::Enum>() {
          this->game = game;
          gamesb4Ad = 1;
    }

    void setup() {
       lastAdTime = -30.;
    }


    ///----------------------------------------------------------------------------//
    ///--------------------- ENTER SECTION ----------------------------------------//
    ///----------------------------------------------------------------------------//
    void onEnter(Scene::Enum) {
#if SAC_DEBUG
        static int minSpanTime = 3;
#else
        static int minSpanTime = 60;
#endif

        float timeSinceLAstAd = TimeUtil::GetTime() - lastAdTime;

        LOGI(gamesb4Ad << " game(s) left. Last ad was " << timeSinceLAstAd << "s ago (min " << minSpanTime << ").");
        
        //if the user cheated and changed this value, reset it to 3
        if (gamesb4Ad > 3) {
            gamesb4Ad = 3;
        // postpone ad if previous ad was shown less than "minSpanTime" sec ago
        } else if (gamesb4Ad <= 0 && timeSinceLAstAd < minSpanTime) {
            LOGI("Last ad was shown earlier than min delay. Postponing this");
            gamesb4Ad = 1;
        // must show an ad
        } else if (gamesb4Ad == 0) {
            //try to launch the ad
            if (game->gameThreadContext->adAPI->showAd(false)) {
                lastAdTime = TimeUtil::GetTime();
            //if not ready, retry next game
            } else {
               LOGI("no ad ready, will retry next game!");
                gamesb4Ad = 1;
            }
        }
    }


    ///----------------------------------------------------------------------------//
    ///--------------------- UPDATE SECTION ---------------------------------------//
    ///----------------------------------------------------------------------------//
    Scene::Enum update(float) {
        if (gamesb4Ad > 0) {
            return Scene::Game;
        } else if(game->gameThreadContext->adAPI->done()) {
            waitAfterAdDisplay = TimeUtil::GetTime();
            return Scene::Game;
        }
        return Scene::Ad;
    }


///----------------------------------------------------------------------------//
///--------------------- EXIT SECTION -----------------------------------------//
///----------------------------------------------------------------------------//
    bool updatePreExit(Scene::Enum, float) {
        if (gamesb4Ad == 0) {
           //wait 1 sec after the ad was showed
            return (TimeUtil::GetTime() - waitAfterAdDisplay >= 1.0);
        } else {
            return true;
        }
    }

    void onExit(Scene::Enum) {
       //decrease gamesb4Ad (-=1 if >0 else =3)
        gamesb4Ad = (gamesb4Ad + 3) % 4;
    }
};

namespace Scene {
    StateHandler<Scene::Enum>* CreateAdSceneHandler(RecursiveRunnerGame* game) {
        return new AdScene(game);
    }
}
