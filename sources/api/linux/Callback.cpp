#include "Callback.h"
#include "api/StorageAPI.h"
#include <string>
#include <vector>

//for strcmp
#include <string.h>
//for atoi
#include <cstdlib>
//for sscanf
#include <stdio.h>


#include <iostream>

//convertit le résultat en une string de la forme "res1, res2, res3, ..."
int callback(void *save, int argc, char **argv, char **azColName __attribute__((unused))){
	std::string *sav = static_cast<std::string*>(save);

	int i = 0;
	for (; i < argc - 1; i++) {
		(*sav) += argv[i];
		(*sav) += ", ";
	}
	(*sav) += argv[i];
		
	return 0;
}

//convertit un tuple en une struct score
int callbackScore(void *save, int argc, char **argv, char **azColName){
	// name | coins | points
	std::vector<StorageAPI::Score> *sav = static_cast<std::vector<StorageAPI::Score>* >(save);
	StorageAPI::Score score;

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


//renvoie le nom des colonnes de la requête
int callbackNames(void *save, int argc, char **argv, char **azColName __attribute__((unused))){
	std::vector<std::string> *sav = static_cast<std::vector<std::string>*>(save);
	for (int i = 0; i < argc; i++) {
		sav->push_back(argv[i]);
	}
	return 0;
}


