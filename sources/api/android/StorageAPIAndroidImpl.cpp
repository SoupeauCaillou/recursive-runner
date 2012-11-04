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

	jmethodID getGameCountBeforeNextAd;
	jmethodID setGameCountBeforeNextAd;

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

	datas->getGameCountBeforeNextAd = jniMethodLookup(env, datas->cls, "getGameCountBeforeNextAd", "()I");
	datas->setGameCountBeforeNextAd = jniMethodLookup(env, datas->cls, "setGameCountBeforeNextAd", "(I)V");

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

int StorageAPIAndroidImpl::getGameCountBeforeNextAd() {
	return env->CallStaticIntMethod(datas->cls, datas->getGameCountBeforeNextAd);
}

void StorageAPIAndroidImpl::setGameCountBeforeNextAd(int inCount) {
	env->CallStaticVoidMethod(datas->cls, datas->setGameCountBeforeNextAd, inCount);
}
