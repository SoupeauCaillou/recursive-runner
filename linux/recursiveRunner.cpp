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
#include "api/linux/StorageAPILinuxImpl.h"
#include "api/linux/CommunicationAPILinuxImpl.h"

#include "systems/TextRenderingSystem.h"
#include "systems/ButtonSystem.h"
#include "systems/TransformationSystem.h"
#include "base/PlacementHelper.h"
#include "DepthLayer.h"

#include "RecursiveRunnerGame.h"
#include "base/Profiler.h"

#include "../sac/util/Recorder.h"
#include <pthread.h>
#include <signal.h>

#ifndef EMSCRIPTEN
#include <locale.h>
#include <libintl.h>
#endif

#define DT 1/60.
#define MAGICKEYTIME 0.15

RecursiveRunnerGame* game;
NameInputAPILinuxImpl* nameInput;
Entity globalFTW = 0;

Recorder *record;

class MouseNativeTouchState: public NativeTouchState {
	public:
		bool isTouching(int index __attribute__((unused)), Vector2* windowCoords) const {
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
        int maxTouchingCount() {
            return 1;
        }
};

#ifndef EMSCRIPTEN
float gameSpeedFactor = 1.0;
void GLFWCALL myCharCallback( int c, int action ) {
	if (globalFTW == 0) {

    } else {
    	if (!TEXT_RENDERING(globalFTW)->hide) {
    		if (action == GLFW_PRESS && (isalnum(c) || c == ' ')) {
    			if (TEXT_RENDERING(globalFTW)->text.length() > 10)
    				return;
    			// filter out all unsupported keystrokes
    			TEXT_RENDERING(globalFTW)->text.push_back((char)c);
    		}
    	}
    }
}

void GLFWCALL myKeyCallback( int key, int action ) {
    if (action != GLFW_RELEASE)
        return;

    if (key == GLFW_KEY_F6) {
        gameSpeedFactor = MathUtil::Min(gameSpeedFactor + 0.1f, 3.0f);
        std::cout << "Game speed: " << gameSpeedFactor << std::endl;
    }
    else if (key == GLFW_KEY_F5) {
        gameSpeedFactor = MathUtil::Max(gameSpeedFactor - 0.1f, 0.0f);
        std::cout << "Game speed: " << gameSpeedFactor << std::endl;
    }
    else if (key == GLFW_KEY_BACKSPACE) {
        if (!TEXT_RENDERING(nameInput->nameEdit)->hide) {
            std::string& text = TEXT_RENDERING(nameInput->nameEdit)->text;
            if (text.length() > 0) {
                text.resize(text.length() - 1);
            }
        } else {
            game->backPressed();
        }
    }
    else if (key == GLFW_KEY_F12) {
        game->togglePause(true);
        uint8_t* out;
        int size = game->saveState(&out);
        if (size) {
            std::ofstream file("/tmp/rr.bin", std::ios_base::binary);
            file.write((const char*)out, size);
            std::cout << "Save state: " << size << " bytes written" << std::endl;
        }
        exit(0);
    }
}

static void updateAndRenderLoop() {
	bool running = true;
	float timer = 0;
	float dtAccumuled=0, dt = 0, time = 0;

	time = TimeUtil::getTime();

	int frames = 0;
	float nextfps = time + 5;
	while(running) {
        game->step();

        running = !glfwGetKey( GLFW_KEY_ESC ) && glfwGetWindowParam( GLFW_OPENED );

        bool focus = glfwGetWindowParam(GLFW_ACTIVE);
        if (focus) {
            theMusicSystem.toggleMute(theSoundSystem.mute);
        } else {
            // theMusicSystem.toggleMute(true);
        }
        //pause ?
        if (glfwGetKey( GLFW_KEY_SPACE )) {// || !focus) {
            if (game->willConsumeBackEvent()) {
                game->backPressed();
            }
        }
        // recording
        if (glfwGetKey( GLFW_KEY_F10) && timer<=0){
            record->stop();
        }
        if (glfwGetKey( GLFW_KEY_F9) && timer<=0){
            record->start();
        }
        //user entered his name?
        if (glfwGetKey( GLFW_KEY_ENTER ) && timer<=0) {
            if (!TEXT_RENDERING(nameInput->nameEdit)->hide) {
                nameInput->textIsReady = true;
            }
        }
    }
	glfwTerminate();
}

static void* callback_thread(void *obj){
	updateAndRenderLoop();
	pthread_exit (0);
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
	Vector2 reso16_10(1000, 625);
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
	glfwSetWindowTitle("RecursiveRunner");
	glewInit();
    __log_enabled = false;
    bool restore = false;
    for (int i=1; i<argc; i++) {
        __log_enabled |= (!strcmp(argv[i], "--verbose") || !strcmp(argv[i], "-v"));
        restore |= !strcmp(argv[i], "-restore");
    }
#endif

	// pose de l'origine du temps ici t = 0
	TimeUtil::init();
	uint8_t* state = 0;
	int size = 0;
	#if 1
	if (restore) {
		FILE* file = fopen("/tmp/rr.bin", "r+b");
		if (file) {
			fseek(file, 0, SEEK_END);
			size = ftell(file);
			fseek(file, 0, SEEK_SET);
			state = new uint8_t[size];
			fread(state, size, 1, file);
			fclose(file);
            std::cout << "Restoring game state from file (size: " << size << ")" << std::endl;
		}
	}
	#endif

	LocalizeAPILinuxImpl* loc = new LocalizeAPILinuxImpl();

	nameInput = new NameInputAPILinuxImpl();

	StorageAPILinuxImpl* storage = new StorageAPILinuxImpl();
	storage->init();

	game = new RecursiveRunnerGame(new AssetAPILinuxImpl(), storage, nameInput, new AdAPI(), new ExitAPILinuxImpl(), new CommunicationAPILinuxImpl());

	theSoundSystem.init();
	theTouchInputManager.setNativeTouchStatePtr(new MouseNativeTouchState());
	MusicAPILinuxOpenALImpl* openal = new MusicAPILinuxOpenALImpl();
    theMusicSystem.musicAPI = openal;
    theMusicSystem.assetAPI = new AssetAPILinuxImpl();
    theRenderingSystem.assetAPI = new AssetAPILinuxImpl();
    SoundAPILinuxOpenALImpl* soundAPI = new SoundAPILinuxOpenALImpl();
    theSoundSystem.soundAPI = soundAPI;
    LOGE("Remove music init");
    openal->init();
    theMusicSystem.init();
    soundAPI->init();

    game->sacInit(reso->X,reso->Y);
    game->init(state, size);

#ifndef EMSCRIPTEN
    setlocale( LC_ALL, "" );
    loc->init();
    glfwSetCharCallback(myCharCallback);
    glfwSetKeyCallback(myKeyCallback);
#endif


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
	record = new Recorder(reso->X, reso->Y);
	pthread_t th1;
	pthread_create (&th1, NULL, callback_thread, NULL);
	while (pthread_kill(th1, 0) == 0)
	{
		game->render();
		glfwSwapBuffers();
		record->record();
	}
#else
	emscripten_set_main_loop(updateAndRender, 60);
#endif
	delete game;
    delete record;
	return 0;
}
