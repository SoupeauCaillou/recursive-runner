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
#include "StorageAPIAndroidImpl.h"
#include "sac/base/Log.h"

static jmethodID jniMethodLookup(JNIEnv* env, jclass c, const std::string& name, const std::string& signature) {
	jmethodID mId = env->GetStaticMethodID(c, name.c_str(), signature.c_str());
	if (!mId) {
		LOGW("JNI Error : could not find method '%s'/'%s'", name.c_str(), signature.c_str());
	}
	return mId;
}

struct StorageAPIAndroidImpl::StorageAPIAndroidImplDatas {
	jclass cls;

	jmethodID submitScore;
	jmethodID getScores;

	jmethodID getCoinsCount;

	jmethodID isFirstGame;
	jmethodID incrementGameCount;

	jmethodID isMuted;
	jmethodID setMuted;

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
	datas->submitScore = jniMethodLookup(env, datas->cls, "submitScore", "(IILjava/lang/String;)V");
	datas->getScores = jniMethodLookup(env, datas->cls, "getScores", "([I[I[Ljava/lang/String;)I");

	datas->getCoinsCount = jniMethodLookup(env, datas->cls, "getCoinsCount", "()I");

	datas->isFirstGame = jniMethodLookup(env, datas->cls, "isFirstGame", "()Z");
	datas->incrementGameCount = jniMethodLookup(env, datas->cls, "incrementGameCount", "()V");

	datas->isMuted = jniMethodLookup(env, datas->cls, "isMuted", "()Z");
	datas->setMuted = jniMethodLookup(env, datas->cls, "setMuted", "(Z)V");

	datas->initialized = true;
}

void StorageAPIAndroidImpl::uninit() {
	if (datas->initialized) {
		env->DeleteGlobalRef(datas->cls);
		datas->initialized = false;
	}
}

void StorageAPIAndroidImpl::submitScore(Score inScr) {
	jstring name = env->NewStringUTF(inScr.name.c_str());

	env->CallStaticVoidMethod(datas->cls, datas->submitScore, inScr.points, inScr.coins, name);
}

std::vector<StorageAPI::Score> StorageAPIAndroidImpl::getScores(float& outAverage) {
	std::vector<StorageAPI::Score> sav;

	// build arrays params
	jintArray points = env->NewIntArray(5);
	jintArray coins = env->NewIntArray(5);
	jobjectArray names = env->NewObjectArray(5, env->FindClass("java/lang/String"), env->NewStringUTF(""));

	jint idummy[5];
	jfloat fdummy[5];
	for (int i=0; i<5; i++) {
		idummy[i] = i;
		fdummy[i] = 2.0 * i;
	}
	env->SetIntArrayRegion(points, 0, 5, idummy);
	env->SetIntArrayRegion(coins, 0, 5, idummy);
	int count = env->CallStaticIntMethod(datas->cls, datas->getScores, points, coins, names);

	for (int i=0; i<count; i++) {
		StorageAPI::Score s;
		env->GetIntArrayRegion(points, i, 1, &s.points);
		env->GetIntArrayRegion(coins, i, 1, &s.coins);
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

	// *** env->GetIntArrayRegion(points, 5, 1, &outAverage);
	return sav;
}

int StorageAPIAndroidImpl::getCoinsCount() {
	return env->CallStaticIntMethod(datas->cls, datas->getCoinsCount);
}

bool StorageAPIAndroidImpl::isFirstGame() {
	return env->CallStaticBooleanMethod(datas->cls, datas->isFirstGame);
}

void StorageAPIAndroidImpl::incrementGameCount() {
	return env->CallStaticVoidMethod(datas->cls, datas->incrementGameCount);
}

bool StorageAPIAndroidImpl::isMuted() const {
    return env->CallStaticBooleanMethod(datas->cls, datas->isMuted);
}

void StorageAPIAndroidImpl::setMuted(bool b) {
    env->CallStaticVoidMethod(datas->cls, datas->setMuted, b);
}
