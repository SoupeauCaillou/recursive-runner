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
int callback(void *save, int argc, char **argv, char **){
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
int callbackNames(void *save, int argc, char **argv, char **){
	std::vector<std::string> *sav = static_cast<std::vector<std::string>*>(save);
	for (int i = 0; i < argc; i++) {
		sav->push_back(argv[i]);
	}
	return 0;
}


