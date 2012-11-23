#pragma once

#include <vector>
#include <string>

class StorageAPI {
	public:
		struct Score {
			int points;
			int coins;
			std::string name;
			
			Score(int inPts = 0, int inCo = 0, std::string inName = "me") : points(inPts), coins(inCo), name(inName) { }
		};

		virtual void submitScore(Score inScore) = 0;
		virtual std::vector<Score> getScores(float& outAverage) = 0;
					
		virtual int getCoinsCount() = 0;

        virtual bool isMuted() const = 0;
        virtual void setMuted(bool b) = 0;

		virtual int getGameCountBeforeNextAd() = 0;
		virtual void setGameCountBeforeNextAd(int inCount) = 0;
};
