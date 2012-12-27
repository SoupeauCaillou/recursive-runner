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

#include "../StorageAPI.h"

class StorageAPILinuxImpl : public StorageAPI {
	public:
		void init();

		void submitScore(Score inScore);
		std::vector<Score> getScores(float& outAverage);

		int getCoinsCount();

		int getGameCountBeforeNextAd();
		void setGameCountBeforeNextAd(int inCount);

		bool isFirstGame();
		void incrementGameCount();

		bool isMuted() const;
		void setMuted(bool b);

	private:
		std::string dbPath;

	#ifdef EMSCRIPTEN
        bool muted;
		std::vector<Score> scores;
	#endif
};
