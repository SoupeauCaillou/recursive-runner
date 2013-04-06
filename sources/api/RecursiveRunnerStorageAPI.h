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

#include <vector>
#include <string>

#include "api/StorageAPI.h"
	
class RecursiveRunnerStorageAPI {
	public:
		struct Score {
			int points;
			int coins;
			std::string name;

			Score(int inPts = 0, int inCo = 0, std::string inName = "me") : points(inPts), coins(inCo), name(inName) { }
		};

		//only used for linux yet
		virtual void init(StorageAPI* storage) = 0;

		virtual int getCoinsCount() = 0;

		virtual void submitScore(Score inScore) = 0;
		virtual std::vector<Score> getScores(float& outAverage) = 0; 
};
