#if SAC_INGAME_EDITORS

#include "RecursiveRunnerDebugConsole.h"

#include "RecursiveRunnerGame.h"

RecursiveRunnerGame* RecursiveRunnerDebugConsole::_game = 0;

void RecursiveRunnerDebugConsole::init(RecursiveRunnerGame* game) {
    _game = game;

    //nothing yet
}
#endif
