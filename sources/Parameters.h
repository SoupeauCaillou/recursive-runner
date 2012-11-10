#pragma once

namespace param {
	//nombre d'aller-retour (defaut = 10)
	const int runner = 10; 
	
	//vitesse de base (defaut = 0.7)
	const float speedConst = 1.1; //0.8;
	
	//vitesse proportionnel au nombre de ghost (defaut = 0.1)
	const float speedCoeff = 0; // 0.08;
}
