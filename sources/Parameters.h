#pragma once

#define COMPUTE_SPEED_FROM_GAME_DURATION(sec) (runner * (20 * 3  + 0.85*2) / (sec))

namespace param {
	//nombre d'aller-retour (defaut = 10)
	const int runner = 10; 
	
	//vitesse de base (defaut = 0.7)
	const float speedConst = COMPUTE_SPEED_FROM_GAME_DURATION(90.5); //6.9; //0.8;
	
	//vitesse proportionnel au nombre de ghost (defaut = 0.1)
	const float speedCoeff = 0; // 0.08;
}
