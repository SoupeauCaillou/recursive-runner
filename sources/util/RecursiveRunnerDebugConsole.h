#if SAC_INGAME_EDITORS && SAC_DEBUG

#include "util/DebugConsole.h"

class RecursiveRunnerGame;

class RecursiveRunnerDebugConsole {
    public:
        static void init(RecursiveRunnerGame* g);

        static void callbackForceEndGame(void* arg);

    private:
        //to interact with the game
        static RecursiveRunnerGame* game;
};

#endif
