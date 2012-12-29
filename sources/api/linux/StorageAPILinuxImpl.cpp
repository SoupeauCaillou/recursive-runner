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
#include "StorageAPILinuxImpl.h"
#include <string>
#include <sstream>
#ifndef EMSCRIPTEN
#include <sqlite3.h>
#endif
#include "base/Log.h"
#include "base/MathUtil.h"
#include "Callback.h"
#include <sys/stat.h>
#include <sys/types.h>

//for getenv
#include <cstdlib>

//for cerr
#include <iostream>

#ifndef EMSCRIPTEN
static bool request(const std::string& dbPath, std::string s, void* res, int (*callbackP)(void*,int,char**,char**)) {
	sqlite3 *db;
	char *zErrMsg = 0;

	int rc = sqlite3_open(dbPath.c_str(), &db);
	if( rc ){
		LOGE("Can't open database %s: %s\n", dbPath.c_str(), sqlite3_errmsg(db));
		sqlite3_close(db);
		return false;
	}

	//si on veut notre callback personnel(script component)
	if (callbackP && res) {
		rc = sqlite3_exec(db, s.c_str(), callbackP, res, &zErrMsg);
	//sinon on prend celui par défaut
	} else {
		rc = sqlite3_exec(db, s.c_str(), callback, res, &zErrMsg);
	}

	if( rc!=SQLITE_OK ){
		LOGE("SQL error: %s (asked = %s)\n", zErrMsg, s.c_str());
		sqlite3_free(zErrMsg);
	}

	sqlite3_close(db);
	return true;
}

static void checkInTable(const std::string & dbPath, const std::string & option,
const std::string & valueIfExist, const std::string & valueIf404) {
	std::string lookFor = "select value from info where opt like '" + option + "'";
	std::string res;

	request(dbPath, lookFor, &res, 0);

	//it doesn't exist yet
	if (res.length() == 0 && valueIf404 != "(null)") {
		lookFor = "insert into info values('" + option + "', '" +
		valueIf404 + "')";
		request(dbPath, lookFor, 0, 0);

	//it exist - need to be updated?
	} else if (res.length() != 0 && valueIfExist != "(null)") {
		lookFor = "update info set value='" + valueIfExist
		+ "' where opt='" + option + "'";
		request(dbPath, lookFor, 0, 0);
	}
}
#endif


void StorageAPILinuxImpl::init() {
	#ifndef EMSCRIPTEN
	std::stringstream ss;
	char * pPath = getenv ("XDG_DATA_HOME");
	if (pPath) {
		ss << pPath;
	} else if ((pPath = getenv ("HOME")) != 0) {
		ss << pPath << "/.local/share/";
	} else {
		ss << "/tmp/";
	}
	ss << "recursiveRunner/";
	dbPath = ss.str();

	// create folder if needed
	struct stat statFolder;
	int st = stat(dbPath.c_str(), &statFolder);
	if (st || (statFolder.st_mode & S_IFMT) != S_IFDIR) {
		if (mkdir(dbPath.c_str(), S_IRWXU | S_IWGRP | S_IROTH)) {
			LOGE("Failed to create : '%s'", dbPath.c_str());
			return;
		}
	}

	ss << "recursiveRunner.db";
	dbPath = ss.str();
	bool r = request(dbPath, "", 0, 0);

	if (r) {
		LOGI("initializing database...");
		request(dbPath, "create table score(points number(7) default '0', coins number(7) default '0', name varchar2(11) default 'Anonymous')", 0, 0);
		request(dbPath, "create table info(opt varchar2(8), value varchar2(11), constraint f1 primary key(opt,value))", 0, 0);

		checkInTable(dbPath, "sound", "(null)", "on");
		checkInTable(dbPath, "gameCount", "(null)", "0");

		//reset gameCount if there is no score in table (no coins collected)
		if (getCoinsCount() == 0) {
			request(dbPath, "update info set value='0' where opt='gameCount'", 0, 0);
		}
	}
    #else
    muted = false;
	#endif
}

void StorageAPILinuxImpl::submitScore(Score inScr) {
	#ifndef EMSCRIPTEN

	//check that the player isn't cheating (adding himself coins) (max is number of coints * runnerCount * runnerghost)
	if (inScr.coins > 20*10) {
		LOGE("you're cheating! %d coins really ?", inScr.coins);
		return;
	}


	std::stringstream tmp;

	tmp << "INSERT INTO score VALUES (" << inScr.points << "," << 2 * inScr.coins + 1 << ",'" << inScr.name << "')";
	request(dbPath, tmp.str().c_str(), 0, 0);

	#else
    int count = MathUtil::Min(5, (int)scores.size());
    for (int i=0; i<count; i++) {
        if (inScr.points > scores[i].points) {
            scores.insert(scores.begin() + i, inScr);
            return;
        }
    }
    if (count < 5)
        scores.push_back(inScr);
	#endif
}

std::vector<StorageAPI::Score> StorageAPILinuxImpl::getScores(float& outAvg) {
	std::vector<StorageAPI::Score> result;

	#ifndef EMSCRIPTEN
	request(dbPath, "select * from score order by points desc limit 5", &result, callbackScore);
    #else
    return scores;
	#endif

	outAvg = -1;
	return result;
}

int StorageAPILinuxImpl::getCoinsCount() {
	#ifndef EMSCRIPTEN
	std::string s;

	//check the table is not empty before
	request(dbPath, "select * from score", &s, 0);
	if (s.empty())
		return 0;

	request(dbPath, "select sum(coins), count(coins) from score", &s, 0);

	int coins, scoreCount;
	sscanf(s.c_str(), "%d, %d", &coins, &scoreCount);
	LOGE("%d, %d", coins, scoreCount);

	return ((coins - scoreCount) / 2.);
	#else
	return 0;
	#endif
}

bool StorageAPILinuxImpl::isFirstGame() {
    #ifndef EMSCRIPTEN
    std::string s;

    request(dbPath, "select value from info where opt like 'gameCount'", &s, 0);

    return (s == "1");
    #else
    return false;
    #endif
}

bool StorageAPILinuxImpl::isMuted() const {
    #ifndef EMSCRIPTEN
    std::string s;
    request(dbPath, "select value from info where opt like 'sound'", &s, 0);
    return (s == "off");
    #else
    return true;
    #endif
}

void StorageAPILinuxImpl::setMuted(bool b) {
    #ifndef EMSCRIPTEN
    std::stringstream req;
    req << "UPDATE info SET value='" << (b ? "off" : "on") << "' where opt='sound'";
    request(dbPath, req.str(),0, 0);
    #else
    muted = b;
    #endif
}
void StorageAPILinuxImpl::incrementGameCount() {
	std::string gameCount;
	request(dbPath, "select value from info where opt='gameCount'", &gameCount, 0);
	LOGE("%s", gameCount.c_str());
	std::stringstream ss;
	ss << atoi(gameCount.c_str()) + 1;

	std::cout << gameCount.c_str() << "->" << ss.str().c_str() << std::endl;
	request(dbPath, "update info set value='" + ss.str() + "' where opt='gameCount'", 0, 0);
}
