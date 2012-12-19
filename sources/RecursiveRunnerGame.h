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

#include <string>
#include <vector>
#include <map>

#include "base/MathUtil.h"
#include "base/Game.h"

#include "systems/RenderingSystem.h"

#include "api/AdAPI.h"
#include "api/ExitAPI.h"
#include "api/StorageAPI.h"
#include "api/CommunicationAPI.h"
#include "states/StateManager.h"

class NameInputAPI;
class SessionComponent;
class LocalizeAPI;

namespace CameraMode {
    enum Enum {
        Menu,
        Single,
    };
}

namespace Level {
    enum Enum {
        Level1,
        Level2
    };
}

class RecursiveRunnerGame : public Game {
   public:
      static void startGame(Level::Enum level, bool transition);
      static void endGame();

   public:
      RecursiveRunnerGame(AssetAPI* ast, StorageAPI* storage, NameInputAPI* nameInput, AdAPI* ad, ExitAPI* exAPI, CommunicationAPI* comAPI, LocalizeAPI* locAPI);
      ~RecursiveRunnerGame();
      void sacInit(int windowW, int windowH);
      void init(const uint8_t* in = 0, int size = 0);
      void quickInit();
      void tick(float dt);
      void backPressed();
      void changeState(State::Enum newState);
      bool willConsumeBackEvent();
      void togglePause(bool pause);

      int saveState(uint8_t** out);
      void setupCamera(CameraMode::Enum mode);
      void updateBestScore();

   private:
      State::Enum currentState, overrideNextState;
      std::map<State::Enum, StateManager*> state2manager;
      TransitionStateManager transitionManager;

      AssetAPI* assetAPI;
      NameInputAPI* nameInputAPI;
      ExitAPI* exitAPI;

      void decor(StorageAPI* storageAPI);
      void initGame(StorageAPI* storageAPI);

      Entity pianist;
   public:
      // shared/global vars
      float baseLine;
      bool ignoreClick;
      Entity silhouette, route, cameraEntity, bestScore;
      std::vector<Entity> decorEntities;
      CommunicationAPI* communicationAPI;
      LocalizeAPI* localizeAPI;
      StorageAPI* storageAPI;
      AdAPI* adAPI;
      // GameTempVar gameTempVars;
      Entity scoreText, scorePanel;
      Entity muteBtn, ground;
      Vector2 leftMostCameraPos;
      Level::Enum level;
      struct {
        float H, V;
      } buttonSpacing;

      static void createCoins(const std::vector<Vector2>& coordinates, SessionComponent* session, bool transition);
};
