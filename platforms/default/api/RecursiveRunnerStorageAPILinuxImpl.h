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

#include "api/RecursiveRunnerStorageAPI.h"
#include "api/linux/CommunicationAPILinuxImpl.h"

class StorageAPILinuxImpl;

class RecursiveRunnerStorageAPILinuxImpl : public RecursiveRunnerStorageAPI, public CommunicationAPI::ScoreHandler {
	public:
		RecursiveRunnerStorageAPILinuxImpl() : _initialized(false) {}
		
		//must be called before anything else
		void init(StorageAPI* storage);

		int getCoinsCount();

		void submitScore(Score inScore);
		std::vector<Score> getScores(float& outAverage);

		std::list<CommunicationAPI::Achievement> getAllAchievements();
		std::list<CommunicationAPI::Score> getScores(unsigned leaderboardID,
			CommunicationAPI::Score::Visibility visibility, unsigned startRank, unsigned count);
		void submitScore(unsigned leaderboardID, CommunicationAPI::Score score);

	private:
#if SAC_EMSCRIPTEN
        std::vector<Score> _scores;
#else
        bool _initialized;
#endif
		StorageAPILinuxImpl* _storageAPI;
};
