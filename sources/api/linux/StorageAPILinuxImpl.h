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

        bool isMuted() const;
        void setMuted(bool b);
		
	private:
		std::string dbPath;
	
	#ifdef EMSCRIPTEN
		Score scores[5];
	#endif
};
