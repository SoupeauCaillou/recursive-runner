#pragma once

#include <vector>
#include <string>

class StorageAPI {
	public:
		struct Score {
			int points;
			std::string name;
			
			Score(int inPts = 0, const std::string& inName = "") : points(inPts), name(inName) { }
		};

		virtual void submitScore(const Score& inScore) = 0;
		virtual std::vector<Score> savedScores() = 0;
		virtual int getGameCountBeforeNextAd() = 0;
		virtual void setGameCountBeforeNextAd(int inCount) = 0;
};
