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
		std::vector<Score> savedScores(float& outAverage);
		int getGameCountBeforeNextAd();
		void setGameCountBeforeNextAd(int inCount);

	private:
		class StorageAPIAndroidImplDatas;
		StorageAPIAndroidImplDatas* datas;
	public:
		JNIEnv* env;
};
