/*
	This file is part of Heriswap.

	@author Soupe au Caillou - Pierre-Eric Pelloux-Prayer
	@author Soupe au Caillou - Gautier Pelloux-Prayer

	Heriswap is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, version 3.

	Heriswap is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with Heriswap.  If not, see <http://www.gnu.org/licenses/>.
*/
#include "StorageAPIAndroidImpl.h"
#include "sac/base/Log.h"

static jmethodID jniMethodLookup(JNIEnv* env, jclass c, const std::string& name, const std::string& signature) {
    jmethodID mId = env->GetStaticMethodID(c, name.c_str(), signature.c_str());
    if (!mId) {
        LOGE("JNI Error : could not find method '%s'/'%s'", name.c_str(), signature.c_str());
    }
    return mId;
}

struct StorageAPIAndroidImpl::StorageAPIAndroidImplDatas {
    jclass cls;

    jmethodID soundEnable;
    jmethodID getGameCountBeforeNextAd;
    jmethodID setGameCountBeforeNextAd;
    jmethodID getSavedGamePointsSum;
    jmethodID submitScore;
    jmethodID getScores;
    jmethodID getModePlayedCount;

    bool initialized;
};

StorageAPIAndroidImpl::StorageAPIAndroidImpl() {
	datas = new StorageAPIAndroidImplDatas();
	datas->initialized = false;
}

StorageAPIAndroidImpl::~StorageAPIAndroidImpl() {
	if (datas->initialized) {
    	env->DeleteGlobalRef(datas->cls);
	}
    delete datas;
}

void StorageAPIAndroidImpl::init(JNIEnv* pEnv) {
	if (datas->initialized) {
		LOGW("StorageAPI not properly uninitialized");
	}
	env = pEnv;

    datas->cls = (jclass)env->NewGlobalRef(env->FindClass("net/damsy/soupeaucaillou/recursiveRunner/api/StorageAPI"));
    datas->soundEnable = jniMethodLookup(env, datas->cls, "soundEnable", "(Z)Z");
    datas->getGameCountBeforeNextAd = jniMethodLookup(env, datas->cls, "getGameCountBeforeNextAd", "()I");
    datas->setGameCountBeforeNextAd = jniMethodLookup(env, datas->cls, "setGameCountBeforeNextAd", "(I)V");
    datas->submitScore = jniMethodLookup(env, datas->cls, "submitScore", "(ILjava/lang/String;)V");
    datas->getScores = jniMethodLookup(env, datas->cls, "getScores", "([I[Ljava/lang/String;)I");

    datas->initialized = true;
}

void StorageAPIAndroidImpl::uninit() {
	if (datas->initialized) {
		env->DeleteGlobalRef(datas->cls);
		datas->initialized = false;
	}
}

void StorageAPIAndroidImpl::submitScore(const Score& inScore) {
    jstring name = env->NewStringUTF(inScore.name.c_str());
    env->CallStaticVoidMethod(datas->cls, datas->submitScore, inScore.points, name);
}

std::vector<StorageAPI::Score> StorageAPIAndroidImpl::savedScores() {
    std::vector<StorageAPI::Score> sav;

    // build arrays params
    jintArray points = env->NewIntArray(5);
    jobjectArray names = env->NewObjectArray(5, env->FindClass("java/lang/String"), env->NewStringUTF(""));

    jint idummy[5];
    for (int i=0; i<5; i++) {
        idummy[i] = i;
    }
    env->SetIntArrayRegion(points, 0, 5, idummy);
    int count = env->CallStaticIntMethod(datas->cls, datas->getScores, points, names);

    for (int i=0; i<count; i++) {
        StorageAPI::Score s;
        env->GetIntArrayRegion(points, i, 1, &s.points);
        jstring n = (jstring)env->GetObjectArrayElement(names, i);
        if (n) {
            const char *mfile = env->GetStringUTFChars(n, 0);
            s.name = (char*)mfile;
            env->ReleaseStringUTFChars(n, mfile);
        } else {
            s.name = "unknown";
        }
        sav.push_back(s);
    }

    return sav;
}

bool StorageAPIAndroidImpl::soundEnable(bool switchIt) {
    return env->CallStaticBooleanMethod(datas->cls, datas->soundEnable, switchIt);
}

int StorageAPIAndroidImpl::getGameCountBeforeNextAd() {
    return env->CallStaticIntMethod(datas->cls, datas->getGameCountBeforeNextAd);
}

void StorageAPIAndroidImpl::setGameCountBeforeNextAd(int c) {
    env->CallStaticVoidMethod(datas->cls, datas->setGameCountBeforeNextAd, c);
}
