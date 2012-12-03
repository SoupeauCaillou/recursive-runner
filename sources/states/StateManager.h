/*
 This file is part of Recursive Runner.

 @author Soupe au Caillou - Pierre-Eric Pelloux-Prayer
 @author Soupe au Caillou - Gautier Pelloux-Prayer

 Heriswap is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, version 3.

 Heriswap is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with Heriswap.  If not, see <http://www.gnu.org/licenses/>.
*/
#pragma once

class RecursiveRunnerGame;

namespace State {
    enum Enum {
        Logo,
        Logo2Menu, // Transition
        Menu,
        Menu2Game, // Transition
        Game,
        Game2Menu, // Transition
        Pause,
        Count
    };
}

class StateManager {
    public:
        StateManager(State::Enum _state, RecursiveRunnerGame* _game) : state(_state), game(_game) {} 
        virtual ~StateManager() {}

        virtual void setup() = 0;
        virtual void earlyEnter() = 0;
        virtual void enter() = 0;
        virtual State::Enum update(float dt) = 0;
        virtual void backgroundUpdate(float dt) = 0;
        virtual void exit() = 0;
        virtual void lateExit() = 0;
        virtual bool transitionCanExit() = 0;
        virtual bool transitionCanEnter() = 0;

        State::Enum state;
    protected:
        RecursiveRunnerGame* game;
};

#define DEF_STATE_MANAGER(state) \
    class state##StateManager : public StateManager {\
        public:\
            state##StateManager(RecursiveRunnerGame* _game);\
            ~state##StateManager();\
            void setup();\
            void earlyEnter();\
            void enter();\
            State::Enum update(float dt);\
            void backgroundUpdate(float dt);\
            void exit();\
            void lateExit();\
            bool transitionCanExit();\
            bool transitionCanEnter();\
        private:\
            class state##StateManagerDatas;\
            state##StateManagerDatas* datas;\
    };\
    
class TransitionStateManager : public StateManager {
    public:
        TransitionStateManager(State::Enum transitionState, StateManager* from, StateManager* to, RecursiveRunnerGame* _game);
        ~TransitionStateManager();
        void setup();
        void earlyEnter() {}
        void enter();
        State::Enum update(float dt);
        void backgroundUpdate(float dt __attribute__((unused))) {}
        void exit();
        void lateExit() {}
        bool transitionCanExit() { return true; }
        bool transitionCanEnter() { return true; }
    private:
        class TransitionStateManagerDatas;
        TransitionStateManagerDatas* datas;
};

DEF_STATE_MANAGER(Logo)
DEF_STATE_MANAGER(Menu)
DEF_STATE_MANAGER(Game)
DEF_STATE_MANAGER(Pause)
