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
#include "StorageAPILinuxImpl.h"

int main(int argc, char** argv) {
    if (initGame("Recursive Runner")) {
        LOGE("Failed to initialize")
        return 1;
    }

    // Init game specific API
    StorageAPILinuxImpl* storage = new StorageAPILinuxImpl();
    storage->init();

    return launchGame(
        new RecursiveRunnerGame(storage),
        argc,
        argv);
}
