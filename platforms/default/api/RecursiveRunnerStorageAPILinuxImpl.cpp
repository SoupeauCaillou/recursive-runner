#include "RecursiveRunnerStorageAPILinuxImpl.h"

#include "base/Log.h"

#include "api/linux/StorageAPILinuxImpl.h"

int RecursiveRunnerStorageAPILinuxImpl::getCoinsCount() {
	LOGE("TODO");
	return 0;
/*#ifdef EMSCRIPTEN
	return 0;
#else
	//check the table is not empty before
	std::string s;
	_storageAPI->request("select * from score", &s, 0);
	if (s.empty())
		return 0;

	_storageAPI->request("select sum(coins), count(coins) from score", &s, 0);

	int coins, partiesDoneCount;

	sscanf(s.c_str(), "%d, %d", &coins, &partiesDoneCount);

	//seems a bit odd (why a sub?)
	return ((coins - partiesDoneCount) / 2.);
#endif
*/
}
