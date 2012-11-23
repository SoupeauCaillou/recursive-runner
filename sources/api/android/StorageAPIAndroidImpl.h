#pragma once

#include "../StorageAPI.h"
#include <jni.h>

class StorageAPIAndroidImpl : public StorageAPI {
	public:
		StorageAPIAndroidImpl();
		~StorageAPIAndroidImpl();
		void init(JNIEnv* env);
		void uninit();
		
		void submitScore(Score inScr);
		std::vector<Score> getScores(float& outAverage);
		
		int getCoinsCount();

		int getGameCountBeforeNextAd();
		void setGameCountBeforeNextAd(int inCount);

        bool isMuted() const;
        void setMuted(bool b);

	private:
		class StorageAPIAndroidImplDatas;
		StorageAPIAndroidImplDatas* datas;
	public:
		JNIEnv* env;
};
