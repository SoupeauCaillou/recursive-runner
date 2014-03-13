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
#include "RecursiveRunnerGame.h"
#include "app/AppSetup.h"

#include <RecursiveRunnerGitVersion.h>
    
int main(int argc, char** argv) {
    std::string versionName = "";
    #if SAC_DEBUG
        versionName = versionName + " / " + TAG_NAME + " - " + VERSION_NAME;
    #endif

    if (initGame("Recursive Runner", glm::ivec2(800, 600), versionName)) {
        LOGE("Failed to initialize");
        return 1;
    }

    return launchGame(
        new RecursiveRunnerGame(),
        argc,
        argv);
}
