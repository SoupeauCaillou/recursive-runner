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


//récupère le nom?
int callback(void *save, int argc __attribute__((unused)), char **argv, char **azColName __attribute__((unused))){
	std::string *sav = static_cast<std::string*>(save);
	if (argv[0] != 0)
		*sav = argv[0];
	return 0;
}

//renvoie le nombre de points d'une ligne de la table
int callbackSc(void *save, int argc, char **argv, char **azColName){
	int *sav = static_cast<int*>(save);
	
	for (int i = 0; i < argc; i++) {
		if (!strcmp(azColName[i], "points")) {
			*sav += std::atoi(argv[i]);
			return 0;
		}
	}
	return 0;
}

//convertit un tuple en une struct score
int callbackScore(void *save, int argc, char **argv, char **azColName){
	// name | points
	std::vector<StorageAPI::Score> *sav = static_cast<std::vector<StorageAPI::Score>* >(save);
	StorageAPI::Score score1;

	for(int i = 0; i < argc; i++){
		if (!strcmp(azColName[i],"name")) {
			score1.name = argv[i];
		} else if (!strcmp(azColName[i],"points")) {
			sscanf(argv[i], "%d", &score1.points);
		}
	}
	sav->push_back(score1);
	return 0;
}


//récupère le nom des colonnes de la table
int callbackNames(void *save, int argc, char **argv, char **azColName __attribute__((unused))){
	std::vector<std::string> *sav = static_cast<std::vector<std::string>*>(save);
	for (int i = 0; i < argc; i++) {
		sav->push_back(argv[i]);
	}
	return 0;
}


