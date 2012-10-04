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
#ifdef EMSCRIPTEN
#include <SDL/SDL.h>
#include <emscripten/emscripten.h>
#else
#define GLEW_STATIC
#include <GL/glew.h>
#include <GL/glfw.h>
#endif
//liste des keys dans /usr/include/GL/glfw.h
#include <sstream>
#include <iostream>
#include <fstream>
#include <cstdlib>
#include <sys/time.h>
#include <algorithm>
#include <sstream>
#include <cassert>
#include <vector>

#include <base/Vector2.h>
#include <base/TouchInputManager.h>
#include <base/TimeUtil.h>
#include <base/MathUtil.h>

#include "systems/RenderingSystem.h"
#include "systems/SoundSystem.h"
#include "systems/MusicSystem.h"

#include "api/linux/MusicAPILinuxOpenALImpl.h"
#include "api/linux/AssetAPILinuxImpl.h"
#include "api/linux/SoundAPILinuxOpenALImpl.h"
#include "api/linux/LocalizeAPILinuxImpl.h"
#include "api/linux/NameInputAPILinuxImpl.h"
#include "api/linux/ExitAPILinuxImpl.h"
#include "api/linux/NetworkAPILinuxImpl.h"

#include "systems/TextRenderingSystem.h"
#include "systems/ButtonSystem.h"
#include "systems/TransformationSystem.h"
#include "base/PlacementHelper.h"
#include "DepthLayer.h"

#include "PrototypeGame.h"
#include "base/Profiler.h"

#ifndef EMSCRIPTEN
#include <locale.h>
#include <libintl.h>
#endif

#define DT 1/60.
#define MAGICKEYTIME 0.15

PrototypeGame* game;
NameInputAPILinuxImpl* nameInput;
Entity globalFTW = 0;

class MouseNativeTouchState: public NativeTouchState {
	public:
		bool isTouching(Vector2* windowCoords) const {
			#ifdef EMSCRIPTEN
			 static bool down = false;
			 static Vector2 position;
			  SDL_Event event;
			  while (SDL_PollEvent(&event)) {
			    switch(event.type) {
			      case SDL_MOUSEMOTION: {
			       	SDL_MouseMotionEvent *m = (SDL_MouseMotionEvent*)&event;
			       	int x,y;
			        SDL_GetMouseState(&x, &y);
			        position.X = x;
			        position.Y = y;
			        break;
			      }
			      case SDL_MOUSEBUTTONDOWN: {
			      	// SDL_GetMouseState(&x, &y);
			        SDL_MouseButtonEvent *m = (SDL_MouseButtonEvent*)&event;
			        if (m->button == SDL_BUTTON_LEFT) {
				        // windowCoords->X = m->x;
						// windowCoords->Y = m->y;
				        down = true;
				        LOGI("Mouse down (%f %f)", windowCoords->X, windowCoords->Y);
			        }
			        break;
			      }
			      case SDL_MOUSEBUTTONUP: {
				    SDL_MouseButtonEvent *m = (SDL_MouseButtonEvent*)&event;
			        if (m->button == SDL_BUTTON_LEFT) {
				    	down = false;
				    	LOGI("Mouse up");
			        }
			        break;
			      }
			      case SDL_KEYDOWN: {
			      	if (globalFTW == 0)
						break;

					if (!TEXT_RENDERING(globalFTW)->hide) {
						char c;
						switch (event.key.keysym.sym) {
							case SDLK_BACKSPACE:
								if (!TEXT_RENDERING(nameInput->nameEdit)->hide) {
									std::string& text = TEXT_RENDERING(nameInput->nameEdit)->text;
									if (text.length() > 0) {
										text.resize(text.length() - 1);
									}
								}
								break;
							case SDLK_RETURN:
								if (!TEXT_RENDERING(nameInput->nameEdit)->hide) {
									nameInput->textIsReady = true;
								}
								break;
							default:
								c = event.key.keysym.sym;
						}

						if (isalnum(c) || c == ' ') {
							if (TEXT_RENDERING(globalFTW)->text.length() > 10)
								break;
							// filter out all unsupported keystrokes
							TEXT_RENDERING(globalFTW)->text.push_back((char)c);
						}
					}
				    break;
			      }
			    }
			   }
			   *windowCoords = position;
			return down;
			#else
			int x,y;
			glfwGetMousePos(&x, &y);
			windowCoords->X = x;
			windowCoords->Y = y;
			return glfwGetMouseButton(GLFW_MOUSE_BUTTON_1) == GLFW_PRESS;
			#endif
		}
};

#ifndef EMSCRIPTEN
void GLFWCALL myCharCallback( int c, int action ) {
	if (globalFTW == 0)
		return;

	if (!TEXT_RENDERING(globalFTW)->hide) {
		if (action == GLFW_PRESS && (isalnum(c) || c == ' ')) {
			if (TEXT_RENDERING(globalFTW)->text.length() > 10)
				return;
			// filter out all unsupported keystrokes
			TEXT_RENDERING(globalFTW)->text.push_back((char)c);
		}
	}
}

static void updateAndRenderLoop() {
	bool running = true;
	float timer = 0;
	float dtAccumuled=0, dt = 0, time = 0;

	time = TimeUtil::getTime();

	bool backIsDown = false;
	int frames = 0;
	float nextfps = time + 5;
	while(running) {
		do {
			dt = TimeUtil::getTime() - time;
			if (dt < DT) {
				struct timespec ts;
				ts.tv_sec = 0;
				ts.tv_nsec = (DT - dt) * 1000000000LL;
				nanosleep(&ts, 0);
			}
		} while (dt < DT);

		if (dt > 1./20) {
			dt = 1./20.;
		}
		dtAccumuled += dt;
		time = TimeUtil::getTime();
		while (dtAccumuled >= DT){
			dtAccumuled -= DT;
			game->tick(DT);
			running = !glfwGetKey( GLFW_KEY_ESC ) && glfwGetWindowParam( GLFW_OPENED );
			bool focus = glfwGetWindowParam(GLFW_ACTIVE);
			if (focus) {
				theMusicSystem.toggleMute(theSoundSystem.mute);
			} else {
				// theMusicSystem.toggleMute(true);
			}
			//pause ?
			if (glfwGetKey( GLFW_KEY_SPACE )) {// || !focus) {
				game->togglePause(true);
			}
			//user entered his name?
			if ((glfwGetKey( GLFW_KEY_ENTER ) || glfwGetKey( GLFW_KEY_KP_ENTER) ) && timer<=0) {
				if (!TEXT_RENDERING(nameInput->nameEdit)->hide) {
					nameInput->textIsReady = true;
				}
				// game.toggleShowCombi(false);
				// timer = MAGICKEYTIME;
			}
			if (glfwGetKey( GLFW_KEY_BACKSPACE)) {
				if (!backIsDown) {
					backIsDown = true;
					// game.backPressed();
					if (!TEXT_RENDERING(nameInput->nameEdit)->hide) {
						std::string& text = TEXT_RENDERING(nameInput->nameEdit)->text;
						if (text.length() > 0) {
							text.resize(text.length() - 1);
						}
					} else {
						game->backPressed();
					}
				}
			} else {
				backIsDown = false;
			}
         #if 0
			if (glfwGetKey( GLFW_KEY_LSHIFT)) {
				uint8_t* state = 0;
				int size = game->saveState(&state);
				if (size) {
					LOGI("ptr: %p %d", state, size);
					FILE* file = fopen("dump.bin", "w+b");
					fwrite(state, size, 1, file);
					fclose(file);
				}
				running = false;
				break;
			}
         #endif
			timer -= DT;
			frames++;
			if (time > nextfps) {
				//std::cout << "FPS: " << (frames / 5) << std::endl;
				nextfps = time + 5;
				frames = 0;
			}
		}

		theRenderingSystem.render();
		glfwSwapBuffers();
	}
	glfwTerminate();
}

#else
static void updateAndRender() {
       static float prevTime = TimeUtil::getTime();
       static float leftOver = 0;
       float t = TimeUtil::getTime();
       float dt = t - prevTime + leftOver;
        SDL_PumpEvents();
       game->tick(dt);
       /*
       while (dt > DT) {
               game->tick(DT);
               dt -= DT;
       }*/
       leftOver = 0;
        theRenderingSystem.render();
        //SDL_GL_SwapBuffers( );
       prevTime = t;
}

#endif

extern bool __log_enabled;
int main(int argc, char** argv) {
	Vector2 reso16_9(394, 700);
	Vector2 reso16_10(800, 500);
	Vector2* reso = &reso16_10;

#ifdef EMSCRIPTEN
	__log_enabled = true;
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0) {
		return 1;
	}

	SDL_Surface *ecran = SDL_SetVideoMode(reso->X, reso->Y, 16, SDL_OPENGL ); /* Double Buffering */
    __log_enabled = false;
#else
	if (!glfwInit())
		return 1;
	glfwOpenWindowHint( GLFW_WINDOW_NO_RESIZE, GL_TRUE );
	if( !glfwOpenWindow( reso->X,reso->Y, 8,8,8,8,8,8, GLFW_WINDOW ) )
		return 1;
	glfwSetWindowTitle("Prototype");
	glewInit();
	__log_enabled = (argc > 1 && !strcmp(argv[1], "--verbose"));
#endif

	// pose de l'origine du temps ici t = 0
	TimeUtil::init();
	uint8_t* state = 0;
	int size = 0;
	#if 0
	if (argc > 1 && !strcmp(argv[1], "-restore")) {
		FILE* file = fopen("dump.bin", "r+b");
		if (file) {
			std::cout << "Restoring game state from file" << std::endl;
			fseek(file, 0, SEEK_END);
			size = ftell(file);
			fseek(file, 0, SEEK_SET);
			state = new uint8_t[size];
			fread(state, size, 1, file);
			fclose(file);
		}
	}
	#endif

	LocalizeAPILinuxImpl* loc = new LocalizeAPILinuxImpl();

	nameInput = new NameInputAPILinuxImpl();

	game = new PrototypeGame(new AssetAPILinuxImpl(), nameInput, loc, new AdAPI(), new ExitAPILinuxImpl());

	theRenderingSystem.opengles2 = true;
	theSoundSystem.init();
	theTouchInputManager.setNativeTouchStatePtr(new MouseNativeTouchState());
	MusicAPILinuxOpenALImpl* openal = new MusicAPILinuxOpenALImpl();
    theMusicSystem.musicAPI = openal;
    theMusicSystem.assetAPI = new AssetAPILinuxImpl();
    theRenderingSystem.assetAPI = new AssetAPILinuxImpl();
    SoundAPILinuxOpenALImpl* soundAPI = new SoundAPILinuxOpenALImpl();
    theSoundSystem.soundAPI = soundAPI;
    LOGE("Remove music init");
    // openal->init();
    theMusicSystem.init();
    // soundAPI->init();

	game->sacInit(reso->X,reso->Y);
	game->init(state, size);

#ifndef EMSCRIPTEN
    setlocale( LC_ALL, "" );
	loc->init();
	glfwSetCharCallback(myCharCallback);
#endif

	Color green = Color(3.0/255.0, 99.0/255, 71.0/255);
	// name input entities
	nameInput->title = theEntityManager.CreateEntity();
	ADD_COMPONENT(nameInput->title, Transformation);
	TRANSFORM(nameInput->title)->position = Vector2(0, PlacementHelper::GimpYToScreen(275));
	TRANSFORM(nameInput->title)->z = DL_HelpText;
	ADD_COMPONENT(nameInput->title, TextRendering);
	TEXT_RENDERING(nameInput->title)->text = loc->text("enter_name", "Enter your name:");
	TEXT_RENDERING(nameInput->title)->fontName = "typo";
	TEXT_RENDERING(nameInput->title)->positioning = TextRenderingComponent::CENTER;
	TEXT_RENDERING(nameInput->title)->color = green;
	TEXT_RENDERING(nameInput->title)->charHeight = PlacementHelper::GimpHeightToScreen(54);
	TEXT_RENDERING(nameInput->title)->hide = true;

	globalFTW = nameInput->nameEdit = theEntityManager.CreateEntity();
	ADD_COMPONENT(nameInput->nameEdit, Transformation);
	TRANSFORM(nameInput->nameEdit)->position = Vector2(0, PlacementHelper::GimpYToScreen(390));
	TRANSFORM(nameInput->nameEdit)->z = DL_HelpText;
	ADD_COMPONENT(nameInput->nameEdit, TextRendering);
	TEXT_RENDERING(nameInput->nameEdit)->fontName = "typo";
	TEXT_RENDERING(nameInput->nameEdit)->positioning = TextRenderingComponent::CENTER;
	TEXT_RENDERING(nameInput->nameEdit)->color = green;
	TEXT_RENDERING(nameInput->nameEdit)->charHeight = PlacementHelper::GimpHeightToScreen(54);
	TEXT_RENDERING(nameInput->nameEdit)->hide = true;
	TEXT_RENDERING(nameInput->nameEdit)->caret.speed = 0.5;

	nameInput->background = theEntityManager.CreateEntity();
	ADD_COMPONENT(nameInput->background, Transformation);
	TRANSFORM(nameInput->background)->size = Vector2(PlacementHelper::GimpWidthToScreen(708), PlacementHelper::GimpHeightToScreen(256));
	TRANSFORM(nameInput->background)->position = Vector2(0, PlacementHelper::GimpYToScreen(320));
	TRANSFORM(nameInput->background)->z = DL_HelpTextBg;
	ADD_COMPONENT(nameInput->background, Rendering);
	RENDERING(nameInput->background)->hide = true;
	RENDERING(nameInput->background)->texture = theRenderingSystem.loadTextureFile("fond_bouton");
	RENDERING(nameInput->background)->color.a = 1;

#if 0
    NetworkAPILinuxImpl* net = new NetworkAPILinuxImpl();
    net->connectToLobby(argv[1], "66.228.34.226");//127.0.0.1");

    while (true) {
        std::cout << "Is connected ? " << net->isConnectedToAnotherPlayer() << std::endl;
        
        struct timespec ts;
        ts.tv_sec = 1;
        ts.tv_nsec = 0;// 0.25 * 1000000000LL;
        nanosleep(&ts, 0);
    }
#endif

#ifndef EMSCRIPTEN
	updateAndRenderLoop();
#else
	emscripten_set_main_loop(updateAndRender, 60);
#endif

	return 0;
}
