#include <SDL2/SDL.h>
#include <chrono>
#include "imageloader.h"
#include "raycaster.h"
#include "color.h"
#include <SDL2/SDL_audio.h>
#include <SDL2/SDL_gamecontroller.h>

SDL_Window* window;
SDL_Renderer* renderer;

typedef struct {
    uint8_t* start;        // Puntero al inicio del archivo de audio
    uint8_t* pos;
    uint32_t length;
    uint32_t totalLength;  // Longitud total del archivo de audio
} AudioData;

void AudioCallback(void* userData, Uint8* stream, int len){
    if(len > 0){
        AudioData* audio = (AudioData*)userData;

        if (audio->length == 0){
            // Reiniciar la reproducción desde el principio
            audio->pos = audio->start;
            audio->length = audio->totalLength;
        }

        len = (len > audio->length ? audio->length : len);

        SDL_memcpy(stream, audio->pos, len);

        audio->pos += len;
        audio->length -= len;
    }
}

void showWelcomeScreen(SDL_Renderer* renderer) {
    bool welcome = false;
    while (!welcome) {
        ImageLoader::render(renderer, "welcome_image", 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
        SDL_RenderPresent(renderer);
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                welcome = true;
            } else if (event.type == SDL_KEYDOWN) {
                if (event.key.keysym.sym == SDLK_SPACE) {
                    welcome = true; // Sal del bucle al presionar la barra espaciadora
                }
            }
        }
    }
}

void clear(SDL_Renderer* renderer){
    SDL_SetRenderDrawColor(renderer, 135, 206, 250, 255);
    SDL_RenderClear(renderer);
}

void draw_floor(SDL_Renderer* renderer, Color floorColor, Color backgroundColor){
    SDL_SetRenderDrawColor(renderer, backgroundColor.r, backgroundColor.g, backgroundColor.b, backgroundColor.a);
    SDL_Rect backgroundRect = {
            0,
            0,
            SCREEN_WIDTH,
            SCREEN_HEIGHT
    };
    SDL_RenderFillRect(renderer, &backgroundRect);
    SDL_SetRenderDrawColor(renderer, floorColor.r, floorColor.g, floorColor.b, floorColor.a);
    SDL_Rect floorRect = {
            0,
            SCREEN_HEIGHT / 2,
            SCREEN_WIDTH,
            SCREEN_HEIGHT / 2
    };
    SDL_RenderFillRect(renderer, &floorRect);
}

void loadMap(Raycaster& r, const std::string& mapFilePath) {
    r.load_map(mapFilePath);
    bool running = true;

    int frameCount = 0;
    double totalTime = 0.0;
    int fps = 0;
    auto startTime = std::chrono::high_resolution_clock::now();

    while (running) {
        SDL_Event event;
        if (SDL_NumJoysticks() > 0) {
            SDL_GameControllerOpen(0); // Abre el primer controlador encontrado
        } else {
            //std::cout << "No se encontraron controladores" << std::endl;
        }
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = false;
                break;
            }
            if (event.type == SDL_KEYDOWN) {
                switch(event.key.keysym.sym) {
                    case SDLK_LEFT:
                        r.player.a += 3.14 / 24;
                        break;
                    case SDLK_RIGHT:
                        r.player.a -= 3.14 / 24;
                        break;
                    case SDLK_UP:
                        r.sdlk_up();
                        break;
                    case SDLK_DOWN:
                        r.sdlk_down();
                        break;
                    default:
                        break;
                }
            }

            // Manejo de eventos del controlador
            if (event.type == SDL_CONTROLLERBUTTONDOWN) {
                switch (event.cbutton.button) {
                    case SDL_CONTROLLER_BUTTON_DPAD_UP:
                        // Acción al presionar el botón arriba del controlador
                        r.sdlk_up();
                        break;
                    case SDL_CONTROLLER_BUTTON_DPAD_DOWN:
                        // Acción al presionar el botón abajo del controlador
                        r.sdlk_down();
                        break;
                        // Agrega más casos para otros botones del controlador
                    case SDL_CONTROLLER_BUTTON_DPAD_RIGHT:
                        r.player.a -= 3.14 / 24;
                        break;
                    case SDL_CONTROLLER_BUTTON_DPAD_LEFT:
                        r.player.a += 3.14 / 24;
                        break;
                }
            }

            const int JOYSTICK_DEAD_ZONE = 32500;

            // Manejo de eventos del joystick
            if (event.type == SDL_JOYAXISMOTION) {
                if (event.jaxis.axis == 0) {
                    if (event.jaxis.value < -JOYSTICK_DEAD_ZONE) {
                        // Mover hacia la izquierda
                        r.player.a += 3.14 / 24;
                    } else if (event.jaxis.value > JOYSTICK_DEAD_ZONE) {
                        // Mover hacia la derecha
                        r.player.a -= 3.14 / 24;
                    }
                }
                if (event.jaxis.axis == 1) {
                    if (event.jaxis.value < -JOYSTICK_DEAD_ZONE) {
                        // Mover hacia arriba
                        r.sdlk_up();
                    } else if (event.jaxis.value > JOYSTICK_DEAD_ZONE) {
                        // Mover hacia abajo
                        r.sdlk_down();
                    }
                }
            }

        }

        clear(renderer);
        draw_floor(renderer, FLOOR_COLOR, BACKGROUND_COLOR);
        r.render();
        SDL_RenderPresent(renderer);

        auto endTime = std::chrono::high_resolution_clock::now();
        double frameTime = std::chrono::duration<double>(endTime - startTime).count();
        startTime = endTime;

        totalTime += frameTime;
        frameCount++;

        if (totalTime >= 1.0) {
            fps = static_cast<int>(frameCount);
            frameCount = 0;
            totalTime = 0.0;
            std::string windowTitle = "DOOM - FPS: " + std::to_string(fps);
            SDL_SetWindowTitle(window, windowTitle.c_str());
        }
    }
}

int main(int argc, char* argv[]) {
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_GAMECONTROLLER);
    ImageLoader::init();

    window = SDL_CreateWindow("DOOM", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

    ImageLoader::loadImage("+", "../assets/foto.png");
    ImageLoader::loadImage("-", "../assets/wall.png");
    ImageLoader::loadImage("|", "../assets/wall.png");
    ImageLoader::loadImage("*", "../assets/foto.png");
    ImageLoader::loadImage(".", "../assets/wall_salida.png");
    ImageLoader::loadImage("welcome_image", "../assets/welcome.png");
    ImageLoader::loadImage("win", "../assets/win.png");
    ImageLoader::loadImage("level_image", "../assets/level.png");

    Raycaster r = { renderer };

    SDL_AudioSpec wavSpec;
    Uint8* wavStart;
    Uint32 wavLength;

    if (SDL_LoadWAV("../assets/diamond_rush.wav", &wavSpec, &wavStart, &wavLength) == NULL) {
        SDL_Log("Error al cargar el archivo de audio: %s\n", SDL_GetError());
        return 1;
    }

    AudioData audioData;
    audioData.start = wavStart;
    audioData.pos = wavStart;
    audioData.length = wavLength;
    audioData.totalLength = wavLength;

    wavSpec.callback = AudioCallback;
    wavSpec.userdata = &audioData;

    if (SDL_OpenAudio(&wavSpec, NULL) < 0) {
        SDL_Log("No se pudo abrir el dispositivo de audio: %s\n", SDL_GetError());
        return 1;
    }

    SDL_PauseAudio(0);

    bool running = true;
    showWelcomeScreen(renderer);
    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            ImageLoader::render(renderer, "level_image", 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
            SDL_RenderPresent(renderer);
            if (event.type == SDL_QUIT) {
                running = false;
                break;
            }
            if (event.type == SDL_KEYDOWN) {
                switch (event.key.keysym.sym) {
                    case SDLK_a:
                        loadMap(r, "../assets/mapita.txt");
                        break;
                    case SDLK_b:
                        loadMap(r, "../assets/mapB.txt");
                        break;
                    case SDLK_c:
                        loadMap(r, "../assets/mapC.txt");
                        break;
                }
            }
        }
    }
    SDL_CloseAudio();
    SDL_FreeWAV(wavStart);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}