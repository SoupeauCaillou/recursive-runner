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
      Tutorial,
      Pause,
      Ad,
      Rate,
      RestartGame,
      SocialCenter,
   };
}
class StateManager {
    public:
        StateManager(State::Enum _state, RecursiveRunnerGame* _game) : state(_state), game(_game) {}
        virtual ~StateManager() {}

        virtual void setup() = 0;
        virtual void willEnter(State::Enum from) = 0;
        virtual void enter(State::Enum from) = 0;
        virtual State::Enum update(float dt) = 0;
        virtual void backgroundUpdate(float dt) = 0;
        virtual void willExit(State::Enum to) = 0;
        virtual void exit(State::Enum to) = 0;
        virtual bool transitionCanExit(State::Enum to) = 0;
        virtual bool transitionCanEnter(State::Enum from) = 0;

        State::Enum state;
    protected:
        RecursiveRunnerGame* game;
};

#define DEF_NEW_STATE(state) \
    class state##State : public StateManager {\
        public:\
            state##State(RecursiveRunnerGame* _game);\
            ~state##State();\
            void setup();\
            void willEnter(State::Enum from);\
            void enter(State::Enum from);\
            State::Enum update(float dt);\
            void backgroundUpdate(float dt);\
            void willExit(State::Enum to);\
            void exit(State::Enum to);\
            bool transitionCanExit(State::Enum from);\
            bool transitionCanEnter(State::Enum to);\
        private:\
            struct state##StateDatas;\
            state##StateDatas* datas;\
    };

class TransitionStateManager {
    public:
        void enter(StateManager* _from, StateManager* _to);
        bool transitionFinished(State::Enum* nextState);
        void exit();
    public:
        StateManager* from, *to;
};

DEF_NEW_STATE(Logo)
DEF_NEW_STATE(Menu)
DEF_NEW_STATE(Game)
DEF_NEW_STATE(Tutorial)
DEF_NEW_STATE(Pause)
DEF_NEW_STATE(Rate)
DEF_NEW_STATE(Ad)
DEF_NEW_STATE(RestartGame)
DEF_NEW_STATE(SocialCenter)
