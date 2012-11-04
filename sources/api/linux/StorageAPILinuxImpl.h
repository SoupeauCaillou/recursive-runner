#pragma once

#include "../StorageAPI.h"

class StorageAPILinuxImpl : public StorageAPI {
	public:
		void init();

		 void submitScore(const Score& inScore);
		 std::vector<Score> savedScores();
		 int getGameCountBeforeNextAd();
		 void setGameCountBeforeNextAd(int inCount);
		
	private:
		std::string dbPath;
	
	#ifdef EMSCRIPTEN
		Score scores[5];
	#endif
};
