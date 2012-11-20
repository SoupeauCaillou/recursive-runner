/*
	This file is part of Heriswap.

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

#include <string>
#include <vector>

#include "base/MathUtil.h"
#include "base/Game.h"

#include "systems/RenderingSystem.h"

#include "api/AdAPI.h"
#include "api/ExitAPI.h"
#include "api/StorageAPI.h"

class NameInputAPI;

class RecursiveRunnerGame : public Game {
	public:
		RecursiveRunnerGame(AssetAPI* ast, StorageAPI* storage, NameInputAPI* nameInput, AdAPI* ad, ExitAPI* exAPI);
        ~RecursiveRunnerGame();
		void sacInit(int windowW, int windowH);
		void init(const uint8_t* in = 0, int size = 0);
		void tick(float dt);
		void togglePause(bool activate);
		void backPressed();

	private:
		AssetAPI* assetAPI;
		StorageAPI* storageAPI;
		NameInputAPI* nameInputAPI;
		ExitAPI* exitAPI;
};
