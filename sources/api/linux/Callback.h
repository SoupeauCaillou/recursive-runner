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
#pragma once

//convertit le résultat en une string de la forme "res1, res2, res3, ..."
int callback(void *save, int argc, char **argv, char **azColName);

//convertit un tuple en une struct score
int callbackScore(void *save, int argc, char **argv, char **azColName);

//renvoie le nom des colonnes de la requête
int callbackNames(void *save, int argc, char **argv, char **azColName);

