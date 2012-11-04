#include "StorageAPILinuxImpl.h"
#include <string>
#include <sstream>
#ifndef EMSCRIPTEN
#include <sqlite3.h>
#endif
#include "base/Log.h"
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
		std::string s;

		s = "";
		request(dbPath, "select value from info where opt like 'gameb4Ads'", &s, 0);
		if (s.length()==0) {
			request(dbPath, "insert into info values('gameb4Ads', '2')", 0, 0);
		} else {
			request(dbPath, "UPDATE info SET value='2' where opt='gameb4Ads'",0, 0);
		}
	}
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
	for (unsigned i = 0; i < 5; i++) {
		if ((scores[i].points == 0 || inScr.points > scores[i].points) {
			
			for (unsigned j = 4; j != i; j--) {
				scores[j] = scores[j-1];
			}
			
			scores[i] = inScr;
			break;
		}
	}
	#endif
}

std::vector<StorageAPI::Score> StorageAPILinuxImpl::getScores(float& outAvg) {
	std::vector<StorageAPI::Score> result;
	
	#ifndef EMSCRIPTEN
	request(dbPath, "select * from score order by points desc limit 5", &result, callbackScore);
   
   #else
	for (unsigned i = 0; i < 5; i++) {
		if (scores[i].points == 0) {
			break;
		} else {
			result.push_back(scores[i]);
		}
	}
	#endif
	
	outAvg = -1;
	return result;
}

int StorageAPILinuxImpl::getCoinsCount() {
	#ifndef EMSCRIPTEN
	std::string s;
	request(dbPath, "select sum(coins), count(coins) from score", &s, 0);
	
	int coins, scoreCount;
	sscanf(s.c_str(), "%d, %d", &coins, &scoreCount);
		
	return ((coins - scoreCount) / 2.);
	#else
	return 0;
	#endif
}

int StorageAPILinuxImpl::getGameCountBeforeNextAd() {
	#ifndef EMSCRIPTEN
	std::string s;
	request(dbPath, "select value from info where opt='gameb4Ads'", &s, 0);
	return std::atoi(s.c_str());
	#else
	return 0;
	#endif
}

void StorageAPILinuxImpl::setGameCountBeforeNextAd(int inCount) {
	#ifndef EMSCRIPTEN
	std::stringstream s;
	s << "update info set value='" << inCount << "' where opt='gameb4Ads'";
	request(dbPath, s.str(),0, 0);
	#endif
}
