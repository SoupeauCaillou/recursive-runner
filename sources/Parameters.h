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

#define COMPUTE_SPEED_FROM_GAME_DURATION(sec) (10 * (20 * 3  + 0.85*2) / (sec))

namespace param {
    const int LevelSize = 3;

    const float CoinScale = 0.6;

	//nombre d'aller-retour (defaut = 10)
	const int runner = 10;

	//vitesse de base (defaut = 0.7)
	const float speedConst = COMPUTE_SPEED_FROM_GAME_DURATION(90.5); //6.9; //0.8;

	//vitesse proportionnel au nombre de ghost (defaut = 0)
	const float speedCoeff = 0; // 0.08;
}
