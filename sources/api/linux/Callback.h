#pragma once

//convertit le résultat en une string de la forme "res1, res2, res3, ..."
int callback(void *save, int argc, char **argv, char **azColName);

//convertit un tuple en une struct score
int callbackScore(void *save, int argc, char **argv, char **azColName);

//renvoie le nom des colonnes de la requête
int callbackNames(void *save, int argc, char **argv, char **azColName);

