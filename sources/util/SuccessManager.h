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
#pragma once

#include <map>
#include <base/Entity.h>

class RecursiveRunnerGame;
struct SessionComponent;


class SuccessManager {
    public:
        void init(RecursiveRunnerGame* g);

        void gameStart(bool bisTuto);
        void oneMoreRunner(int scoreLastRunner);
        void oneLessRunner();
        void gameEnd(SessionComponent* sc);
    private:
        RecursiveRunnerGame * game;
        
        bool isTuto;

        // 0. switch all the lights in a row (INCREMENTAL)
        int bestLighter;
        int achievement0CurrentStep;
        // 1. have 4 living runners
        // 2. have 6 living runners
        // 3. have 8 living runners
        int totalLiving;
        int maxLiving;
        // 4. open 7/20 lights with each runner (INCREMENTAL)
        int totalGoodLighters;
        int achievement4CurrentStep;
        // 5. reach a total score of 50K
        // 6. reach a total score of 100K
        // 7. reach a total score of 200K
};
