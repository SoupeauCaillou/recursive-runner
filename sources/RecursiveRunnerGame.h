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
#include <glm/glm.hpp>
#include "base/Game.h"

#include "systems/RenderingSystem.h"

#include "api/AdAPI.h"
#include "api/ExitAPI.h"
#include "api/CommunicationAPI.h"

#include "util/GameCenterAPIHelper.h"
#include "util/SuccessManager.h"

#include "states/Scenes.h"

#include "base/StateMachine.h"
class NameInputAPI;
struct SessionComponent;
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
#ifdef SAC_INGAME_EDITORS
    friend class RecursiveRunnerDebugConsole;
#endif

   public:
      static void startGame(Level::Enum level, bool transition);
      static void endGame();

   public:
      RecursiveRunnerGame();
      ~RecursiveRunnerGame();
      bool wantsAPI(ContextAPI::Enum api) const;
      void sacInit(int windowW, int windowH);
      void init(const uint8_t* in = 0, int size = 0);
      void quickInit();
      void tick(float dt);
      void backPressed();
      bool willConsumeBackEvent();
      void togglePause(bool pause);

      int saveState(uint8_t** out);
      void setupCamera(CameraMode::Enum mode);
      void updateBestScore();

   private:
      void decor();
      void initGame();


    private:
#if SAC_DEBUG
    public:
#endif
        StateMachine<Scene::Enum> sceneStateMachine;

   public:
      // shared/global vars
      float baseLine;
      bool ignoreClick;
      Entity silhouette, route, cameraEntity, bestScore;
      std::vector<Entity> decorEntities;
      GameCenterAPIHelper gamecenterAPIHelper;
      SuccessManager successManager;

      // GameTempVar gameTempVars;
      Entity scoreText, scorePanel;
      Entity muteBtn, ground;
      Entity pianist;
      glm::vec2 leftMostCameraPos;
      Level::Enum level;
      struct {
        float H, V;
      } buttonSpacing;

      static float nextRunnerStartTime[100];
      static int nextRunnerStartTimeIndex;

      static void createCoins(const std::vector<glm::vec2>& coordinates, SessionComponent* session, bool transition);
};
