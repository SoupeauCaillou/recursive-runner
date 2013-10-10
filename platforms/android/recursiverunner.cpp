/*
    This file is part of Dogtag.

    @author Soupe au Caillou - Jordane Pelloux-Prayer
    @author Soupe au Caillou - Gautier Pelloux-Prayer
    @author Soupe au Caillou - Pierre-Eric Pelloux-Prayer

    Dogtag is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, version 3.

    Dogtag is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Dogtag.  If not, see <http://www.gnu.org/licenses/>.
*/



#include "android/sacjnilib.h"
#include "../sources/RecursiveRunnerGame.h"
/*
class DogtagGameThreadJNIEnvCtx : public GameThreadJNIEnvCtx {
	public:

    void init(JNIEnv* pEnv, jobject assetMgr) {
	    GameThreadJNIEnvCtx::init(pEnv, assetMgr);
    }

    void uninit(JNIEnv* pEnv) {
		if (env == pEnv) {
		}
		GameThreadJNIEnvCtx::uninit(pEnv);
    }
};
*/
GameHolder* GameHolder::build() {
	GameHolder* hld = new GameHolder();

/*
	DogtagGameThreadJNIEnvCtx* jniCtx = new DogtagGameThreadJNIEnvCtx();
	hld->gameThreadJNICtx = jniCtx;
*/
	hld->game = new RecursiveRunnerGame();
	return hld;
};
