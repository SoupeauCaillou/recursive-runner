#include "RecursiveRunnerStorageAPILinuxImpl.h"

#include "base/Log.h"

#include "api/linux/StorageAPILinuxImpl.h"

#include <sstream>

///////////////////////////////////////////////////////////////////////////////
///////////////////////Callbacks for sqlite datas treatment////////////////////
///////////////////////////////////////////////////////////////////////////////
#include <string.h>

	//convert a tuple into a score struct
	int callbackScore(void *save, int argc, char **argv, char **azColName){
	    // name | coins | points
	    std::vector<RecursiveRunnerStorageAPI::Score> *sav = static_cast<std::vector<RecursiveRunnerStorageAPI::Score>* >(save);
	    RecursiveRunnerStorageAPI::Score score;

	    for(int i = 0; i < argc; i++){
	        if (!strcmp(azColName[i],"points")) {
	            sscanf(argv[i], "%d", &score.points);
	        } else if (!strcmp(azColName[i],"coins")) {
	            sscanf(argv[i], "%d", &score.coins);
	        } else if (!strcmp(azColName[i],"name")) {
	            score.name = argv[i];
	        }
	    }
	    sav->push_back(score);
	    return 0;
	}
///////////////////////////////////////////////////////////////////////////////
///////////////////////End of callbacks////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void RecursiveRunnerStorageAPILinuxImpl::init(StorageAPI* storage) {
	_initialized = true;
	_storageAPI = (StorageAPILinuxImpl*) storage;
	_storageAPI->request("create table score(points number(7) default '0', coins number(7) default '0', name varchar2(11) default 'Anonymous')", 0, 0);
}

int RecursiveRunnerStorageAPILinuxImpl::getCoinsCount() {
#ifdef EMSCRIPTEN
	return 0;
#else
	LOGF_IF(!_initialized, "Didn't initialized RecursiveRunnerStorageAPI");

	//check the table is not empty before
	std::string s;
	_storageAPI->request("select * from score", &s, 0);
	if (s.empty())
		return 0;

	_storageAPI->request("select sum(coins), count(coins) from score", &s, 0);

	int coins, partiesDoneCount;

	sscanf(s.c_str(), "%d, %d", &coins, &partiesDoneCount);

	//that seems a bit odd (why a sub?)
	return ((coins - partiesDoneCount) / 2.);
#endif
}

void RecursiveRunnerStorageAPILinuxImpl::submitScore(Score inScr) {
#if SAC_EMSCRIPTEN
    int count = MathUtil::Min(5, (int)_scores.size());
    for (int i=0; i<count; i++) {
        if (inScr.points > _scores[i].points) {
            _scores.insert(_scores.begin() + i, inScr);
            return;
        }
    }
    if (count < 5)
        _scores.push_back(inScr);

#else
	LOGF_IF(!_initialized, "Didn't initialized RecursiveRunnerStorageAPI");

    //check that the player isn't cheating (adding himself coins) (max is number of coints * runnerCount * runnerghost)
    if (inScr.coins > 20*10) {
        LOGE("you're cheating! " << inScr.coins << " coins really ?")
        return;
    }

    std::stringstream statement;

    statement << "INSERT INTO score VALUES (" << inScr.points << "," << 2 * inScr.coins + 1 << ",'" << inScr.name << "')";
     _storageAPI->request(statement.str().c_str(), 0, 0);

#endif
}

std::vector<RecursiveRunnerStorageAPI::Score> RecursiveRunnerStorageAPILinuxImpl::getScores(float& outAvg) {
#if SAC_EMSCRIPTEN
    return _scores;
#else
	LOGF_IF(!_initialized, "Didn't initialized RecursiveRunnerStorageAPI");

    std::vector<RecursiveRunnerStorageAPI::Score> result;
    _storageAPI->request("select * from score order by points desc limit 5", &result, callbackScore);
	outAvg = -1;
	return result;
#endif
}

 
std::list<CommunicationAPI::Achievement> RecursiveRunnerStorageAPILinuxImpl::getAllAchievements() {
	std::list<CommunicationAPI::Achievement> list;
	LOGW("TODO");
	return list;
}

std::list<CommunicationAPI::Score> RecursiveRunnerStorageAPILinuxImpl::getScores(unsigned /*leaderboardID*/,
	CommunicationAPI::Score::Visibility /*visibility*/, unsigned startRank, unsigned count) {
	
	std::list<CommunicationAPI::Score> list;

	float avg; //wont be used
	unsigned rank = 0;
	for (auto entry : getScores(avg)) {
		++rank;
		if (rank < startRank) 
			continue;
		else if (rank - startRank > count) 
			break;

		list.push_back(CommunicationAPI::Score(entry.name, entry.points, rank, CommunicationAPI::Score::ME));
	}
	return list;
}


void RecursiveRunnerStorageAPILinuxImpl::submitScore(unsigned /*leaderboardID*/, CommunicationAPI::Score score) {
	Score newScore;
	newScore.points = atoi(score._score.c_str());
	newScore.name = score._name;
	newScore.coins = 0;

	submitScore(newScore);	
}


