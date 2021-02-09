/*
 SPDX-License-Identifier: MIT
 kgchess
 https://github.com/kgabis/kgchess
 Copyright (c) 2021 Krzysztof Gabis
 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:
 The above copyright notice and this permission notice shall be included in
 all copies or substantial portions of the Software.
 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 THE SOFTWARE.
 */

#include <assert.h>
#include <string.h>
#include <stdbool.h>
#include <limits.h>
#include <time.h>

#ifdef WIN32
#include <SDL.h>
#include <SDL_image.h>
#else
#include <SDL2/SDL.h>
#include <SDL2_image/SDL_image.h>
#endif

#include "../kgchess.h"

#define BOARD_SIZE 320
#define BOARD_MARGIN 10
#define PIECE_SIZE (BOARD_SIZE / 8)

typedef enum game_state {
    GAME_STATE_NONE = 0,
    GAME_STATE_SELECT,
    GAME_STATE_MOVE,
    GAME_STATE_PROMOTION,
} game_state_t;

typedef struct game {
    SDL_Renderer *renderer;
    SDL_Texture *pieces_texture;
    kgchess_t *chess;
    game_state_t state;
    int cursor_x;
    int cursor_y;
    kgchess_moves_array_t moves;
    kgchess_moves_array_t attacks;
} game_t;

static game_t* game_make(SDL_Renderer *renderer);
static bool game_on_clicked(game_t *game, int x, int y);
static void game_render(game_t *game);
static void highlight_field(game_t *game, int x, int y);
static SDL_Rect get_field_rect(game_t *game, int x, int y);

static int chessai_rate_move(kgchess_t *chess, kgchess_move_t move);
static bool chessai_move(kgchess_t *chess);

int main(int argc, char *argv[]) {
    srand((unsigned int)time(NULL));

    if (SDL_Init(SDL_INIT_EVERYTHING) != 0) {
        fprintf(stderr, "Initializing SDL failed.\n");
        return 1;
    }

    int window_size = BOARD_SIZE + 2 * BOARD_MARGIN;

    SDL_Window *window = SDL_CreateWindow("kgchess",
                                          SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                          window_size, window_size, SDL_WINDOW_ALLOW_HIGHDPI);
    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

    int win_width = 0, win_height = 0;
    SDL_GL_GetDrawableSize(window, &win_width, &win_height);
    SDL_RenderSetScale(renderer, (float)win_width / window_size, (float)win_height / window_size);
    
    game_t *game = game_make(renderer);

    while (true) {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            switch (e.type) {
                case SDL_QUIT: {
                    goto loop_end;
                    break;
                }
                case SDL_MOUSEBUTTONUP: {
                    game_on_clicked(game, e.button.x, e.button.y);
                    break;
                }
                default: break;
            }
        }

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0xff);
        SDL_RenderClear(renderer);

        game_render(game);

        SDL_RenderPresent(renderer);
    }
loop_end:
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}

static game_t* game_make(SDL_Renderer *renderer) {
    game_t *game = malloc(sizeof(game_t));
    memset(game, 0, sizeof(game_t));
    game->renderer = renderer;
    game->pieces_texture = IMG_LoadTexture(renderer, "pieces.png");
    assert(game->pieces_texture);
    game->chess = kgchess_make();
    if (rand() % 2) {
        chessai_move(game->chess);
    }
    game->state = GAME_STATE_SELECT;
    game->cursor_x = -1;
    game->cursor_y = -1;
    game->moves = kgchess_moves_array_make_empty();
    return game;
}

static bool game_on_clicked(game_t *game, int mx, int my) {
    int x = (mx - BOARD_MARGIN) / PIECE_SIZE;
    int y = 7 - (my - BOARD_MARGIN) / PIECE_SIZE;

    switch (game->state) {
        case GAME_STATE_SELECT: {
            kgchess_piece_t piece = kgchess_get_piece_at(game->chess, x, y);
            if (piece.type == KGCHESS_PIECE_NONE) {
                break;
            }
            if (piece.player != kgchess_get_current_player(game->chess)) {
                break;
            }
            game->moves = kgchess_get_moves(game->chess, x, y);
            game->state = GAME_STATE_MOVE;
            game->cursor_x = x;
            game->cursor_y = y;
            break;
        }
        case GAME_STATE_MOVE: {
            bool move_found = false;
            for (int i = 0; i < game->moves.count; i++) {
                kgchess_move_t move = game->moves.items[i];
                if (x == move.to.x && y == move.to.y) {
                    kgchess_move(game->chess, move);
                    move_found = true;
                    break;
                }
            }
            if (!move_found) {
                game->state = GAME_STATE_SELECT;
                break;
            }
            game->moves = kgchess_moves_array_make_empty();
            kgchess_state_t chess_state = kgchess_get_state(game->chess);
            if (chess_state == KGCHESS_STATE_PROMOTION) {
                game->state = GAME_STATE_PROMOTION;
            } else {
                game->state = GAME_STATE_SELECT;
                chessai_move(game->chess);

            }
            game->cursor_x = -1;
            game->cursor_y = -1;
            break;
        }
        case GAME_STATE_PROMOTION: {
            if (y != 7) {
                break;
            }
            bool ok = kgchess_promote(game->chess, x);
            if (ok) {
                game->state = GAME_STATE_SELECT;
                chessai_move(game->chess);
            }
        }
        default: break;
    }

    return true;
}

static void game_render(game_t *game) {
    int tex_w, tex_h;
    SDL_QueryTexture(game->pieces_texture, NULL, NULL, &tex_w, &tex_h);
    const int piece_tex_w = tex_w / 6;
    const int piece_tex_h = tex_h / 2;

    const SDL_Rect piece_src_positions[] = {
        { 0, 0, 0 },
        { 0 * piece_tex_w, 0, piece_tex_w, piece_tex_h },
        { 1 * piece_tex_w, 0, piece_tex_w, piece_tex_h },
        { 2 * piece_tex_w, 0, piece_tex_w, piece_tex_h },
        { 3 * piece_tex_w, 0, piece_tex_w, piece_tex_h },
        { 4 * piece_tex_w, 0, piece_tex_w, piece_tex_h },
        { 5 * piece_tex_w, 0, piece_tex_w, piece_tex_h },
    };

    SDL_SetRenderDrawColor(game->renderer, 0xcc, 0xcc, 0xcc, 0xff);
    SDL_RenderClear(game->renderer);

    SDL_RenderFillRect(game->renderer, &(SDL_Rect){ BOARD_MARGIN, BOARD_MARGIN, BOARD_SIZE, BOARD_SIZE});

    for (int x = 0; x < 8; x++) {
        for (int y = 0; y < 8; y++) {
            if ((x + y) % 2 != 0) {
                SDL_SetRenderDrawColor(game->renderer, 0xdd, 0xdd, 0xdd, 0xff);
            } else {
                SDL_SetRenderDrawColor(game->renderer, 0x88, 0x88, 0x88, 0xff);
            }
            SDL_Rect r = get_field_rect(game, x, y);
            SDL_RenderFillRect(game->renderer, &r);
        }
    }

    if (game->state == GAME_STATE_MOVE) {
        highlight_field(game, game->cursor_x, game->cursor_y);
        for (int i = 0; i < game->moves.count; i++) {
            kgchess_move_t move = game->moves.items[i];
            highlight_field(game, move.to.x, move.to.y);
        }
    }

    if (game->state == GAME_STATE_PROMOTION) {
        for (int i = KGCHESS_PIECE_QUEEN; i < KGCHESS_PIECE_PAWN; i++) {
            SDL_Rect piece_src = piece_src_positions[i];
            if (kgchess_get_current_player(game->chess) == KGCHESS_PLAYER_BLACK) {
                piece_src.y += piece_tex_h;
            }
            SDL_Rect dst = get_field_rect(game, i, 7);
            SDL_RenderCopy(game->renderer, game->pieces_texture, &piece_src, &dst);
        }
    } else {
        for (int x = 0; x < 8; x++) {
            for (int y = 0; y < 8; y++) {
                kgchess_piece_t piece = kgchess_get_piece_at(game->chess, x, y);
                if (piece.type == KGCHESS_PIECE_NONE) {
                    continue;
                }
                SDL_Rect piece_src = piece_src_positions[piece.type];
                if (piece.player == KGCHESS_PLAYER_BLACK) {
                    piece_src.y += piece_tex_h;
                }
                SDL_Rect dst = get_field_rect(game, x, y);
                SDL_RenderCopy(game->renderer, game->pieces_texture, &piece_src, &dst);
            }
        }
    }
}

static void highlight_field(game_t *game, int x, int y) {
    SDL_SetRenderDrawColor(game->renderer, 0x21, 0xb2, 0x11, 0x7b);
    int field_margin = 3;
    SDL_Rect r = get_field_rect(game, x, y);
    r.x += field_margin;
    r.y += field_margin;
    r.w -= field_margin * 2;
    r.h -= field_margin * 2;
    SDL_RenderFillRect(game->renderer, &r);
}

static SDL_Rect get_field_rect(game_t *game, int x, int y) {
    return (SDL_Rect){ BOARD_MARGIN + x * PIECE_SIZE, BOARD_MARGIN + (7 - y) * PIECE_SIZE, PIECE_SIZE, PIECE_SIZE };
}

static int chessai_rate_move(kgchess_t *chess, kgchess_move_t move) {
    kgchess_player_t player = kgchess_get_current_player(chess);
    kgchess_player_t enemy = kgchess_get_enemy_player(player);
    int result = 0;
    if (kgchess_is_square_attacked_by_player(chess, move.to.x, move.to.y, enemy)) {
        result -= 20;
    }
    if (move.is_attack) {
        result += 10;
    }
    result += (rand()%20-10);
    return result;
}

static bool chessai_move(kgchess_t *chess) {
    int best_move_score = INT_MIN;
    kgchess_move_t best_move = {};
    kgchess_player_t player = kgchess_get_current_player(chess);
    for (int x = 0; x < 8; x++) {
        for (int y = 0; y < 8; y++) {
            kgchess_piece_t p = kgchess_get_piece_at(chess, x, y);
            if (p.player != player) {
                continue;
            }
            kgchess_moves_array_t moves = kgchess_get_moves(chess, x, y);
            for (int i = 0; i < moves.count; i++) {
                kgchess_move_t move = moves.items[i];
                int score = chessai_rate_move(chess, move);
                if (score > best_move_score) {
                    best_move = move;
                    best_move_score = score;
                }
            }
        }
    }

    if (best_move_score == INT_MIN) {
        return false;
    }
    kgchess_move(chess, best_move);
    return true;
}
