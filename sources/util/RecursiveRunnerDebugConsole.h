#if SAC_INGAME_EDITORS

#include "util/DebugConsole.h"

class RecursiveRunnerGame;

class RecursiveRunnerDebugConsole {
    public:
        static void init(RecursiveRunnerGame* game);

    private:
        //to interact with the game
        static RecursiveRunnerGame* _game;
};

#endif
