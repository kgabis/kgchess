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

#ifndef kgchess_h
#define kgchess_h

#ifdef __cplusplus
extern "C"
{
#endif
#if 0
} // unconfuse xcode
#endif

#include <stdint.h>
#include <stdbool.h>

#define KGCHESS_VERSION_MAJOR 0
#define KGCHESS_VERSION_MINOR 1
#define KGCHESS_VERSION_PATCH 0

#define KGCHESS_VERSION_STRING "0.1.0"

typedef enum {
    KGCHESS_PIECE_NONE = 0,
    KGCHESS_PIECE_KING,
    KGCHESS_PIECE_QUEEN,
    KGCHESS_PIECE_BISHOP,
    KGCHESS_PIECE_KNIGHT,
    KGCHESS_PIECE_ROOK,
    KGCHESS_PIECE_PAWN,
} kgchess_piece_type_t;

typedef enum {
    KGCHESS_PLAYER_NONE = 0,
    KGCHESS_PLAYER_WHITE,
    KGCHESS_PLAYER_BLACK
} kgchess_player_t;

typedef enum {
    KGCHESS_STATE_NONE = 0,
    KGCHESS_STATE_MOVE,
    KGCHESS_STATE_PROMOTION,
    KGCHESS_STATE_ENDED,
} kgchess_state_t;

typedef struct kgchess_piece {
    kgchess_piece_type_t type;
    kgchess_player_t player;
} kgchess_piece_t;

typedef struct kgchess_pos {
    int8_t x;
    int8_t y;
} kgchess_pos_t;

typedef struct kgchess_move {
    kgchess_pos_t from;
    kgchess_pos_t to;
    bool is_castling;
    bool is_attack;
    bool is_en_passant;
} kgchess_move_t;

typedef struct kgchess_moves_array {
    kgchess_move_t items[28];
    int count;
} kgchess_moves_array_t;

typedef struct kgchess kgchess_t;

kgchess_t* kgchess_make(void);
kgchess_t* kgchess_make_copy(const kgchess_t *chess);
void kgchess_destroy(kgchess_t *chess);
kgchess_piece_t kgchess_get_piece_at(const kgchess_t *chess, int x, int y);
kgchess_moves_array_t kgchess_moves_array_make_empty(void);
kgchess_moves_array_t kgchess_get_moves(const kgchess_t *chess, int x, int y);
bool kgchess_move(kgchess_t *chess, kgchess_move_t move);
kgchess_player_t kgchess_get_enemy_player(kgchess_player_t player);
kgchess_state_t kgchess_get_state(kgchess_t *chess);
kgchess_pos_t kgchess_get_promotion_position(kgchess_t *chess);
bool kgchess_promote(kgchess_t *chess, kgchess_piece_type_t piece_type);
kgchess_player_t kgchess_get_winner(kgchess_t *chess);
kgchess_player_t kgchess_get_current_player(kgchess_t *chess);
void kgchess_draw(kgchess_t *chess);
void kgchess_set_winner(kgchess_t *chess, kgchess_player_t player);
bool kgchess_is_square_attacked_by_player(const kgchess_t *chess, int x, int y, kgchess_player_t player);

#ifdef __cplusplus
}
#endif

#endif // kgchess_h
