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

	jmethodID getGameCountBeforeNextAd;
	jmethodID setGameCountBeforeNextAd;
	jmethodID submitScore;
	jmethodID getScores;

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

	datas->cls = (jclass)env->NewGlobalRef(env->FindClass("net/damsy/soupeaucaillou/heriswap/api/StorageAPI"));
	datas->getGameCountBeforeNextAd = jniMethodLookup(env, datas->cls, "getGameCountBeforeNextAd", "()I");
	datas->setGameCountBeforeNextAd = jniMethodLookup(env, datas->cls, "setGameCountBeforeNextAd", "(I)V");
	datas->submitScore = jniMethodLookup(env, datas->cls, "submitScore", "(IIIIFLjava/lang/String;)V");
	datas->getScores = jniMethodLookup(env, datas->cls, "getScores", "(II[I[I[F[Ljava/lang/String;)I");

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
	/*todo*/
	env->CallStaticVoidMethod(datas->cls, datas->submitScore, inScr.points, name);
}

std::vector<StorageAPI::Score> StorageAPIAndroidImpl::savedScores(float& outAverage) {
	/* to do 
	
	
	std::vector<StorageAPI::Score> sav;

	// build arrays params
	jintArray points = env->NewIntArray(5);
	jintArray levels = env->NewIntArray(5);
	jfloatArray times = env->NewFloatArray(6);
	jobjectArray names = env->NewObjectArray(5, env->FindClass("java/lang/String"), env->NewStringUTF(""));

	jint idummy[5];
	jfloat fdummy[5];
	for (int i=0; i<5; i++) {
		idummy[i] = i;
		fdummy[i] = 2.0 * i;
	}
	env->SetIntArrayRegion(points, 0, 5, idummy);
	env->SetIntArrayRegion(levels, 0, 5, idummy);
	env->SetFloatArrayRegion(times, 0, 5, fdummy);
	int count = env->CallStaticIntMethod(datas->cls, datas->getScores, (int)mode, (int)difficulty, points, levels, times, names);

	for (int i=0; i<count; i++) {
		StorageAPI::Score s;
		env->GetIntArrayRegion(points, i, 1, &s.points);
		env->GetIntArrayRegion(levels, i, 1, &s.level);
		env->GetFloatArrayRegion(times, i, 1, &s.time);
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
	
	env->GetFloatArrayRegion(times, 5, 1, &average);
	*/
	return sav;
}

int StorageAPIAndroidImpl::getGameCountBeforeNextAd() {
	return env->CallStaticIntMethod(datas->cls, datas->getGameCountBeforeNextAd);
}

void StorageAPIAndroidImpl::setGameCountBeforeNextAd(int inCount) {
	env->CallStaticVoidMethod(datas->cls, datas->setGameCountBeforeNextAd, inCount);
}
