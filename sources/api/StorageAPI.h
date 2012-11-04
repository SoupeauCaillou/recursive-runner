#pragma once

#include <vector>
#include <string>

class StorageAPI {
	public:
		struct Score {
			int points;
			std::string name;
			
			Score(int inPts = 0, std::string inName = "") : points(inPts), name(inName) { }
		};

		virtual void submitScore(Score inScore) = 0;
		virtual std::vector<Score> savedScores(float& outAverage) = 0;
		virtual int getGameCountBeforeNextAd() = 0;
		virtual void setGameCountBeforeNextAd(int inCount) = 0;
};
