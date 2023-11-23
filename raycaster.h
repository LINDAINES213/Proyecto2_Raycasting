#pragma once
#include <iostream>
#include <fstream>
#include <SDL_render.h>
#include <string>
#include <vector>
#include <cmath>
#include <SDL2/SDL.h>
#include <unordered_map>
#include "color.h"
#include "imageloader.h"

const Color B = {0, 0, 0};
const Color W = {255, 255, 255};
const Color BACKGROUND_COLOR = {21, 177, 229};
const Color FLOOR_COLOR = {9, 69, 153};
const Color MAP = {19, 160, 204};
const int WIDTH = 16;
const int HEIGHT = 11;
const int BLOCK = 50;
const int SCREEN_WIDTH = WIDTH * BLOCK;
const int SCREEN_HEIGHT = HEIGHT * BLOCK;
const int BLOCKSIZE = 17;
const int MAPWIDTH = BLOCKSIZE * WIDTH;
const int MAPHEIGHT = BLOCKSIZE * HEIGHT;

struct Player {
  int x;
  int y;
  float a;
  float fov;
};

struct Impact {
  float d;
  std::string mapHit;  // + | -
  int tx;
};

class Raycaster {
public:
    Raycaster(SDL_Renderer* renderer)
    : renderer(renderer) {

        player.x = BLOCKSIZE + BLOCKSIZE / 2;
        player.y = BLOCKSIZE + BLOCKSIZE / 2;

        player.a = M_PI / 4.0f;
        player.fov = M_PI / 3.0f;

        scale = 25;
        tsize = 103;
    }

    void sdlk_up(){
        int speed = 10;
        // Calcula la nueva posición después de moverse hacia adelante
        int newX = static_cast<int>(player.x + speed * cos(player.a));
        int newY = static_cast<int>(player.y + speed * sin(player.a));

        // Convierte la nueva posición en índices de matriz
        int newI = newX / BLOCKSIZE;
        int newJ = newY / BLOCKSIZE;

        // Verifica si la nueva posición es un espacio en blanco en el mapa
        if (map[newJ][newI] == ' ') {
            // Actualiza la posición del jugador
            player.x = newX;
            player.y = newY;
        }
    }

    void sdlk_down(){
        int speed = 10;
        // Calcula la nueva posición después de moverse hacia adelante
        int newX = static_cast<int>(player.x - speed * cos(player.a));
        int newY = static_cast<int>(player.y - speed * sin(player.a));

        // Convierte la nueva posición en índices de matriz
        int newI = newX / BLOCKSIZE;
        int newJ = newY / BLOCKSIZE;

        // Verifica si la nueva posición es un espacio en blanco en el mapa
        if (map[newJ][newI] == ' ') {
            // Actualiza la posición del jugador
            player.x = newX;
            player.y = newY;
        }
    }

    void load_map(const std::string& filename) {
        std::ifstream file(filename);
        std::string line;
        while (getline(file, line)) {
          map.push_back(line);
        }
        file.close();
    }

    void point(int x, int y, Color c) {
        SDL_SetRenderDrawColor(renderer, c.r, c.g, c.b, c.a);
        SDL_RenderDrawPoint(renderer, x, y);
    }

    void rect(int x, int y, const std::string& mapHit) {
        for(int cx = x; cx < x + BLOCKSIZE; cx++) {
            for(int cy = y; cy < y + BLOCKSIZE; cy++) {
                int tx = ((cx - x) * tsize) / BLOCKSIZE;
                int ty = ((cy - y) * tsize) / BLOCKSIZE;

                Color c = ImageLoader::getPixelColor(mapHit, tx, ty);
                SDL_SetRenderDrawColor(renderer, c.r, c.g, c.b , 255);
                SDL_RenderDrawPoint(renderer, cx, cy);
            }
        }
    }

    Impact cast_ray(float a) {
        float d = 0;
        std::string mapHit;
        int tx;

        while(true) {
            int x = static_cast<int>(player.x + d * cos(a));
            int y = static_cast<int>(player.y + d * sin(a));

            int i = static_cast<int>(x / BLOCKSIZE);
            int j = static_cast<int>(y / BLOCKSIZE);


            if (map[j][i] != ' ') {
                mapHit = map[j][i];
                int hitx = x - i * BLOCKSIZE;
                int hity = y - j * BLOCKSIZE;
                int maxhit;

                if (hitx == 0 || hitx == BLOCK - 1) {
                    maxhit = hity;
                } else {
                    maxhit = hitx;
                }
                tx = maxhit * tsize / BLOCK;
                break;
            }
            point(x, y, W);
            d += 1;
        }
        return Impact{d, mapHit, tx};
    }

    void draw_stake(int x, float h, Impact i) {
        float start = SCREEN_HEIGHT/2.0f - h/2.0f;
        float end = start + h;

        for (int y = start; y < end; y++) {
            int ty = (y - start) * tsize / h;
            Color c = ImageLoader::getPixelColor(i.mapHit, i.tx, ty);
            SDL_SetRenderDrawColor(renderer, c.r, c.g, c.b, c.a);
            SDL_RenderDrawPoint(renderer, x, y);
        }
    }

    // Función para verificar si el jugador ha ganado
    bool check_win_condition() {
        int player_x = static_cast<int>(player.x / BLOCK);
        int player_y = static_cast<int>(player.y / BLOCK);

        std::cout << "Player Position: (" << player_x << ", " << player_y << ")" << std::endl;

        if ((player_x == 4 && player_y == 3) || (player_x == 5 && player_y == 2)) {
            std::cout << "You Win!" << std::endl;
            return true;
        }
        return false;
    }


    void render() {

        if (check_win_condition()) {
            // Limpia el renderizador
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
            SDL_RenderClear(renderer);

            // Dibuja la pantalla de victoria (imagen o texto)
            ImageLoader::render(renderer, "win", 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);

            // Refresca el renderizador
            SDL_RenderPresent(renderer);

            // Pausa el juego por un breve momento para que el jugador vea la pantalla de victoria
            SDL_Delay(10000);  // Puedes ajustar la duración según tus preferencias
            //SDL_Quit();
            //exit(0);
        } else {
        // draw left side of the screen
            for (int i = 1; i < SCREEN_WIDTH; i++) {
                  double a = player.a + player.fov / 2.0 - player.fov * i / SCREEN_WIDTH;
                  Impact impact = cast_ray(a);
                  float d = impact.d;
                  Color c = Color(255, 0, 0);

                  if (d == 0) {
                      std::cout << "you lose" << std::endl;
                      exit(1);
                  }

                  int x = i;
                  float h = static_cast<float>(SCREEN_HEIGHT)/static_cast<float>(d) * static_cast<float>(scale);
                  draw_stake(x, h, impact);
            }


            for (int x = 0; x < MAPWIDTH; x += BLOCKSIZE) {
                for (int y = 0; y < MAPHEIGHT; y += BLOCKSIZE) {
                    int i = static_cast<int>(x / BLOCKSIZE);
                    int j = static_cast<int>(y / BLOCKSIZE);

                    if (map[j][i] != ' ') {
                      std::string mapHit;
                      mapHit = map[j][i];
                      Color c = Color(255, 0, 0);
                      rect(x, y, mapHit);
                    } else{
                        SDL_SetRenderDrawColor(renderer, MAP.r, MAP.g, MAP.b, MAP.a);
                        SDL_Rect rect = {x, y, BLOCKSIZE, BLOCKSIZE};
                        SDL_RenderFillRect(renderer, &rect);
                    }
                }
          }}

        for (int i = 0; i < MAPWIDTH; i++) {
          float a = player.a + player.fov / 2 - player.fov * i / MAPWIDTH;
          cast_ray(a);
        }
      }
Player player;
private:
  int scale;
  SDL_Renderer* renderer;

  std::vector<std::string> map;
  int tsize;
};