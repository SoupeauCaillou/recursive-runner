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

class RecursiveRunnerGame;

namespace State {
   enum Enum {
      Invalid,
      Transition,
      Logo,
      Menu,
      Game,
      Pause,
      Ad,
      Rate,
   };
}

class StateManager {
    public:
        StateManager(State::Enum _state, RecursiveRunnerGame* _game) : state(_state), game(_game) {}
        virtual ~StateManager() {}

        virtual void setup() = 0;
        virtual void willEnter() = 0;
        virtual void enter() = 0;
        virtual State::Enum update(float dt) = 0;
        virtual void backgroundUpdate(float dt) = 0;
        virtual void willExit() = 0;
        virtual void exit() = 0;
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
            void willEnter();\
            void enter();\
            State::Enum update(float dt);\
            void backgroundUpdate(float dt);\
            void willExit();\
            void exit();\
            bool transitionCanExit();\
            bool transitionCanEnter();\
        private:\
            class state##StateManagerDatas;\
            state##StateManagerDatas* datas;\
    };\

class TransitionStateManager {
    public:
        void enter(StateManager* _from, StateManager* _to);
        bool transitionFinished(State::Enum* nextState);
        void exit();
    private:
        StateManager* from, *to;
};

DEF_STATE_MANAGER(Logo)
DEF_STATE_MANAGER(Menu)
DEF_STATE_MANAGER(Game)
DEF_STATE_MANAGER(Pause)
DEF_STATE_MANAGER(Rate)
DEF_STATE_MANAGER(Ad)
