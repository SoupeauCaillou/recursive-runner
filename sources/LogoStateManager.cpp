/*
	This file is part of Heriswap.

	@author Soupe au Caillou - Pierre-Eric Pelloux-Prayer
	@author Soupe au Caillou - Gautier Pelloux-Prayer

	Heriswap is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, version 3.

	Heriswap is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with Heriswap.  If not, see <http://www.gnu.org/licenses/>.
*/
#include "LogoStateManager.h"

#include <base/EntityManager.h>
#include <base/TouchInputManager.h>
#include <base/PlacementHelper.h>

#include "systems/TransformationSystem.h"
#include "systems/RenderingSystem.h"
#include "systems/SoundSystem.h"

#include "DepthLayer.h"

LogoStateManager::LogoStateManager() {}

void LogoStateManager::Setup() {
     logo = theEntityManager.CreateEntity();
     logobg = theEntityManager.CreateEntity();
     logofade = theEntityManager.CreateEntity();

     ADD_COMPONENT(logo, Rendering);
     ADD_COMPONENT(logo, Transformation);
     TRANSFORM(logo)->position = Vector2(0,0);
     TRANSFORM(logo)->size = Vector2(PlacementHelper::ScreenHeight * 0.8, PlacementHelper::ScreenHeight * 0.8);
     TRANSFORM(logo)->z = DL_Logo;
     RENDERING(logo)->texture = theRenderingSystem.loadTextureFile("soupe_logo");

     ADD_COMPONENT(logobg, Rendering);
     ADD_COMPONENT(logobg, Transformation);
     TRANSFORM(logobg)->position = Vector2(0,0);
     TRANSFORM(logobg)->size = Vector2(PlacementHelper::ScreenWidth, PlacementHelper::ScreenHeight);
     RENDERING(logobg)->color = Color(0,0,0);
     TRANSFORM(logobg)->z = DL_BehindLogo;

     ADD_COMPONENT(logofade, Rendering);
     ADD_COMPONENT(logofade, Transformation);
     TRANSFORM(logofade)->position = Vector2(0,0);
     TRANSFORM(logofade)->size = Vector2(PlacementHelper::ScreenWidth, PlacementHelper::ScreenHeight);
     RENDERING(logofade)->color = Color(0,0,0);
     TRANSFORM(logofade)->z = 1;
}

void LogoStateManager::Enter() {
	animLogo = theEntityManager.CreateEntity();
	ADD_COMPONENT(animLogo, Transformation);
	TRANSFORM(animLogo)->size = TRANSFORM(logo)->size * theRenderingSystem.getTextureSize("soupe_logo2_365_331") 
        * Vector2(1.0 / theRenderingSystem.getTextureSize("soupe_logo").X, 1.0 / theRenderingSystem.getTextureSize("soupe_logo").Y);
    Vector2 offset = Vector2(-10 / 800.0, 83/869.0) * TRANSFORM(logo)->size;
    TRANSFORM(animLogo)->position = TRANSFORM(logo)->position + offset;
	TRANSFORM(animLogo)->z = DL_LogoAnim;
	ADD_COMPONENT(animLogo, Rendering);
	RENDERING(animLogo)->texture = theRenderingSystem.loadTextureFile("soupe_logo2_365_331");
	RENDERING(animLogo)->hide = true;
	duration = 0;
	ADD_COMPONENT(animLogo, Sound);
    RENDERING(logo)->hide = RENDERING(logobg)->hide = RENDERING(logofade)->hide = false;
    // preload sound
    theSoundSystem.loadSoundFile("son_monte.ogg");
    step = LogoStep0;
}

#define FADE 0.5
int LogoStateManager::Update(float dt) {
    duration += dt;

    switch (step) {
        case LogoStep0:
            RENDERING(logofade)->color.a = 1 - (duration / FADE);
            if (duration > FADE) {
                duration = 0;
                RENDERING(logofade)->hide = true;
                step = LogoStep1;
            }
            break;
        case LogoStep1:
            if (duration > 0.8) {
                duration = 0;
                RENDERING(animLogo)->hide = false;
                SOUND(animLogo)->sound = theSoundSystem.loadSoundFile("son_monte.ogg");
                step = LogoStep2;
            }
            break;
        case LogoStep2:
            if (duration > 0.05) {
                duration = 0;
                RENDERING(animLogo)->texture = theRenderingSystem.loadTextureFile("soupe_logo3_365_331");
                step = LogoStep3;
            }
            break;
        case LogoStep3:
            if (duration > 0.25) {
                duration = 0;
                RENDERING(animLogo)->texture = theRenderingSystem.loadTextureFile("soupe_logo2_365_331");
                step = LogoStep4;
            }
            break;
        case LogoStep4:
            if (duration > 0.05) {
                duration = 0;
                RENDERING(animLogo)->hide = true;
                step = LogoStep5;
            }
            break;
        case LogoStep5:
            if (duration > 0.6) {
                duration = 0;
                RENDERING(logofade)->hide = false;
                step = LogoStep6;
            }
            break;
        case LogoStep6:
            RENDERING(logofade)->color.a = (duration / FADE);
            if (duration > FADE) {
                duration = 0;
                theEntityManager.DeleteEntity(logo);
                theEntityManager.DeleteEntity(logobg);
                theEntityManager.DeleteEntity(animLogo);
                step = LogoStep7;
                return 1;
            }
            break;
        case LogoStep7:
            RENDERING(logofade)->color.a = 1 - (duration / (4 * FADE));
            if (duration > 4*FADE) {
                duration = 0;
                RENDERING(logofade)->hide = true;
                step = LogoStep1;
                return 2;
            }
            break;
    }
    return 0;
}

void LogoStateManager::Exit() {
    // ou unloadLogo
    theRenderingSystem.unloadAtlas("logo");

    theEntityManager.DeleteEntity(logofade);
}
