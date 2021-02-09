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


#include "kgchess.h"

#include <string.h>
#include <stdlib.h>

#define ARRAY_LENGTH(array) (sizeof((array))/sizeof((array)[0]))

typedef struct {
    kgchess_piece_type_t type;
    kgchess_player_t player;
    int last_move_num;
} kgchess_piece_internal_t;

typedef struct kgchess {
    kgchess_player_t current_player;
    kgchess_piece_internal_t pieces[8][8];
    int move_num;
    kgchess_move_t last_move;
    kgchess_state_t state;
    kgchess_pos_t promotion_pos;
    kgchess_player_t winner;
} kgchess_t;

static kgchess_pos_t KGCHESS_POS_INVALID = (kgchess_pos_t){ -1, -1 };

//-----------------------------------------------------------------------------
// Private declarations
//-----------------------------------------------------------------------------

static kgchess_move_t move_make(int from_x, int from_y, int to_x, int to_y, bool is_attack, bool is_castling, bool is_en_passant);
static void add_move(kgchess_moves_array_t *arr, kgchess_move_t move);
static void add_move_if_legal(const kgchess_t *chess, kgchess_moves_array_t *arr, kgchess_move_t move, bool is_attacks_check);
static void apply_move(kgchess_t *chess, kgchess_move_t move, bool update_state);
static bool is_position_empty(const kgchess_t *chess, int x, int y);
static bool is_player_at_position(const kgchess_t *chess, kgchess_player_t player, int x, int y);
static kgchess_piece_internal_t piece_make(kgchess_piece_type_t type, kgchess_player_t player);
static kgchess_piece_internal_t get_piece_at(const kgchess_t *chess, int x, int y);
static void set_piece_at(kgchess_t *chess, kgchess_piece_internal_t piece, int x, int y);
static bool is_castling_possible(const kgchess_t *chess, int x, int y, int rook_x);
static int get_en_passant(const kgchess_t *chess, int x, int y, kgchess_piece_t piece);
static bool is_in_check(const kgchess_t *chess, kgchess_player_t player);
static void check_checkmate(kgchess_t *chess);

static kgchess_piece_t convert_piece(kgchess_piece_internal_t piece);

static kgchess_moves_array_t get_moves(const kgchess_t *chess, int x, int y, bool add_potential_attacks, bool is_attacks_check);
static kgchess_moves_array_t get_king_moves(const kgchess_t *chess, int x, int y, kgchess_piece_t piece, bool add_potential_attacks, bool is_attacks_check);
static kgchess_moves_array_t get_queen_moves(const kgchess_t *chess, int x, int y, kgchess_piece_t piece, bool add_potential_attacks, bool is_attacks_check);
static kgchess_moves_array_t get_bishop_moves(const kgchess_t *chess, int x, int y, kgchess_piece_t piece, bool add_potential_attacks, bool is_attacks_check);
static kgchess_moves_array_t get_knight_moves(const kgchess_t *chess, int x, int y, kgchess_piece_t piece, bool add_potential_attacks, bool is_attacks_check);
static kgchess_moves_array_t get_rook_moves(const kgchess_t *chess, int x, int y, kgchess_piece_t piece, bool add_potential_attacks, bool is_attacks_check);
static kgchess_moves_array_t get_pawn_moves(const kgchess_t *chess, int x, int y, kgchess_piece_t piece, bool add_potential_attacks, bool is_attacks_check);
static void add_move_range(const kgchess_t *chess, kgchess_moves_array_t *moves, int x, int y, kgchess_piece_t piece,
                            int dx, int dy, int end, bool add_potential_attacks, bool is_attacks_check);

//-----------------------------------------------------------------------------
// Public definitions
//-----------------------------------------------------------------------------

kgchess_t* kgchess_make() {
    kgchess_t *chess = malloc(sizeof(kgchess_t));
    memset(chess, 0, sizeof(kgchess_t));
    set_piece_at(chess, piece_make(KGCHESS_PIECE_ROOK,   KGCHESS_PLAYER_WHITE), 0, 0);
    set_piece_at(chess, piece_make(KGCHESS_PIECE_KNIGHT, KGCHESS_PLAYER_WHITE), 1, 0);
    set_piece_at(chess, piece_make(KGCHESS_PIECE_BISHOP, KGCHESS_PLAYER_WHITE), 2, 0);
    set_piece_at(chess, piece_make(KGCHESS_PIECE_QUEEN,  KGCHESS_PLAYER_WHITE), 3, 0);
    set_piece_at(chess, piece_make(KGCHESS_PIECE_KING,   KGCHESS_PLAYER_WHITE), 4, 0);
    set_piece_at(chess, piece_make(KGCHESS_PIECE_BISHOP, KGCHESS_PLAYER_WHITE), 5, 0);
    set_piece_at(chess, piece_make(KGCHESS_PIECE_KNIGHT, KGCHESS_PLAYER_WHITE), 6, 0);
    set_piece_at(chess, piece_make(KGCHESS_PIECE_ROOK,   KGCHESS_PLAYER_WHITE), 7, 0);

    set_piece_at(chess, piece_make(KGCHESS_PIECE_PAWN, KGCHESS_PLAYER_WHITE), 0, 1);
    set_piece_at(chess, piece_make(KGCHESS_PIECE_PAWN, KGCHESS_PLAYER_WHITE), 1, 1);
    set_piece_at(chess, piece_make(KGCHESS_PIECE_PAWN, KGCHESS_PLAYER_WHITE), 2, 1);
    set_piece_at(chess, piece_make(KGCHESS_PIECE_PAWN, KGCHESS_PLAYER_WHITE), 3, 1);
    set_piece_at(chess, piece_make(KGCHESS_PIECE_PAWN, KGCHESS_PLAYER_WHITE), 4, 1);
    set_piece_at(chess, piece_make(KGCHESS_PIECE_PAWN, KGCHESS_PLAYER_WHITE), 5, 1);
    set_piece_at(chess, piece_make(KGCHESS_PIECE_PAWN, KGCHESS_PLAYER_WHITE), 6, 1);
    set_piece_at(chess, piece_make(KGCHESS_PIECE_PAWN, KGCHESS_PLAYER_WHITE), 7, 1);

    set_piece_at(chess, piece_make(KGCHESS_PIECE_ROOK,   KGCHESS_PLAYER_BLACK), 0, 7);
    set_piece_at(chess, piece_make(KGCHESS_PIECE_KNIGHT, KGCHESS_PLAYER_BLACK), 1, 7);
    set_piece_at(chess, piece_make(KGCHESS_PIECE_BISHOP, KGCHESS_PLAYER_BLACK), 2, 7);
    set_piece_at(chess, piece_make(KGCHESS_PIECE_QUEEN,  KGCHESS_PLAYER_BLACK), 3, 7);
    set_piece_at(chess, piece_make(KGCHESS_PIECE_KING,   KGCHESS_PLAYER_BLACK), 4, 7);
    set_piece_at(chess, piece_make(KGCHESS_PIECE_BISHOP, KGCHESS_PLAYER_BLACK), 5, 7);
    set_piece_at(chess, piece_make(KGCHESS_PIECE_KNIGHT, KGCHESS_PLAYER_BLACK), 6, 7);
    set_piece_at(chess, piece_make(KGCHESS_PIECE_ROOK,   KGCHESS_PLAYER_BLACK), 7, 7);

    set_piece_at(chess, piece_make(KGCHESS_PIECE_PAWN, KGCHESS_PLAYER_BLACK), 0, 6);
    set_piece_at(chess, piece_make(KGCHESS_PIECE_PAWN, KGCHESS_PLAYER_BLACK), 1, 6);
    set_piece_at(chess, piece_make(KGCHESS_PIECE_PAWN, KGCHESS_PLAYER_BLACK), 2, 6);
    set_piece_at(chess, piece_make(KGCHESS_PIECE_PAWN, KGCHESS_PLAYER_BLACK), 3, 6);
    set_piece_at(chess, piece_make(KGCHESS_PIECE_PAWN, KGCHESS_PLAYER_BLACK), 4, 6);
    set_piece_at(chess, piece_make(KGCHESS_PIECE_PAWN, KGCHESS_PLAYER_BLACK), 5, 6);
    set_piece_at(chess, piece_make(KGCHESS_PIECE_PAWN, KGCHESS_PLAYER_BLACK), 6, 6);
    set_piece_at(chess, piece_make(KGCHESS_PIECE_PAWN, KGCHESS_PLAYER_BLACK), 7, 6);

    chess->current_player = KGCHESS_PLAYER_WHITE;
    chess->move_num = 0;
    chess->state = KGCHESS_STATE_MOVE;
    chess->promotion_pos = KGCHESS_POS_INVALID;
    chess->winner = KGCHESS_PLAYER_NONE;

    return chess;
}

kgchess_t* kgchess_make_copy(const kgchess_t *chess) {
    kgchess_t *res = kgchess_make();
    memcpy(res, chess, sizeof(kgchess_t));
    return res;
}

void kgchess_destroy(kgchess_t *chess) {
    free(chess);
}

kgchess_piece_t kgchess_get_piece_at(const kgchess_t *chess, int x, int y) {
    return convert_piece(get_piece_at(chess, x, y));
}

kgchess_moves_array_t kgchess_moves_array_make_empty() {
    kgchess_moves_array_t arr;
    memset(&arr, 0, sizeof(kgchess_moves_array_t));
    return arr;
}

kgchess_moves_array_t kgchess_get_moves(const kgchess_t *chess, int x, int y) {
    return get_moves(chess, x, y, false, false);
}

bool kgchess_move(kgchess_t *chess, kgchess_move_t move) {
    apply_move(chess, move, true);
    return true;
}

kgchess_player_t kgchess_get_enemy_player(kgchess_player_t player) {
    if (player == KGCHESS_PLAYER_BLACK) {
        return KGCHESS_PLAYER_WHITE;
    } else if (player == KGCHESS_PLAYER_WHITE) {
        return KGCHESS_PLAYER_BLACK;
    } else {
        return KGCHESS_PLAYER_NONE;
    }
}

kgchess_state_t kgchess_get_state(kgchess_t *chess) {
    return chess->state;
}

kgchess_pos_t kgchess_get_promotion_position(kgchess_t *chess) {
    return chess->promotion_pos;
}

bool kgchess_promote(kgchess_t *chess, kgchess_piece_type_t piece_type) {
    if (chess->state != KGCHESS_STATE_PROMOTION) {
        return false;
    }
    if (chess->promotion_pos.x == -1 || chess->promotion_pos.y == -1) {
        return false;
    }
    if (piece_type == KGCHESS_PIECE_PAWN || piece_type == KGCHESS_PIECE_KING) {
        return false;
    }
    kgchess_piece_internal_t piece = get_piece_at(chess, chess->promotion_pos.x, chess->promotion_pos.y);
    piece.type = piece_type;
    set_piece_at(chess, piece, chess->promotion_pos.x, chess->promotion_pos.y);
    chess->state = KGCHESS_STATE_MOVE;
    chess->current_player = kgchess_get_enemy_player(chess->current_player);
    check_checkmate(chess);
    return true;
}

kgchess_player_t kgchess_get_winner(kgchess_t *chess) {
    return chess->winner;
}

kgchess_player_t kgchess_get_current_player(kgchess_t *chess) {
    return chess->current_player;
}

void kgchess_draw(kgchess_t *chess) {
    chess->state = KGCHESS_STATE_ENDED;
    chess->winner = KGCHESS_PLAYER_NONE;
}

void kgchess_set_winner(kgchess_t *chess, kgchess_player_t player) {
    chess->state = KGCHESS_STATE_ENDED;
    chess->winner = player;
}

bool kgchess_is_square_attacked_by_player(const kgchess_t *chess, int square_x, int square_y, kgchess_player_t player) {
    for (int x = 0; x < 8; x++) {
        for (int y = 0; y < 8; y++) {
            kgchess_piece_t piece = kgchess_get_piece_at(chess, x, y);
            if (piece.player != player) {
                continue;
            }
            kgchess_moves_array_t moves = get_moves(chess, x, y, true, true);
            for (int i = 0; i < moves.count; i++) {
                kgchess_move_t move = moves.items[i];
                if (move.is_attack && move.to.x == square_x && move.to.y == square_y) {
                    return true;
                }
            }
        }
    }
    return false;
}

//-----------------------------------------------------------------------------
// Private definitions
//-----------------------------------------------------------------------------

static kgchess_move_t move_make(int from_x, int from_y, int to_x, int to_y, bool is_attack, bool is_castling, bool is_en_passant) {
    kgchess_move_t move;
    move.from.x = from_x;
    move.from.y = from_y;
    move.to.x = to_x;
    move.to.y = to_y;
    move.is_attack = is_attack;
    move.is_castling = is_castling;
    move.is_en_passant = is_en_passant;
    return move;
}

static void add_move(kgchess_moves_array_t *arr, kgchess_move_t move) {
    if (arr->count >= ARRAY_LENGTH(arr->items)) {
        return;
    }
    arr->items[arr->count] = move;
    arr->count++;
}

static void add_move_if_legal(const kgchess_t *chess, kgchess_moves_array_t *arr, kgchess_move_t move, bool is_attacks_check) {
    if (move.to.x < 0 || move.to.x >= 8 || move.to.y < 0 || move.to.y >= 8) {
        return;
    }
    if (!is_attacks_check) {
        kgchess_piece_t piece = kgchess_get_piece_at(chess, move.from.x, move.from.y);
        kgchess_t chess_copy = *chess;
        apply_move(&chess_copy, move, false);
        if (is_in_check(&chess_copy, piece.player)) {
            return;
        }
    }
    add_move(arr, move);
}

static void apply_move(kgchess_t *chess, kgchess_move_t move, bool update_state) {
    kgchess_piece_internal_t empty_piece = piece_make(KGCHESS_PIECE_NONE, KGCHESS_PLAYER_NONE);

    if (move.is_castling) {
        kgchess_piece_internal_t king = get_piece_at(chess, move.from.x, move.from.y);
        int rook_from_x = -1;
        int rook_to_x = -1;
        if (move.to.x == 2) {
            rook_from_x = 0;
            rook_to_x = 3;
        } else {
            rook_from_x = 7;
            rook_to_x = 5;
        }
        kgchess_piece_internal_t rook = get_piece_at(chess, rook_from_x, move.from.y);

        set_piece_at(chess, empty_piece, move.from.x, move.from.y);
        set_piece_at(chess, empty_piece, rook_from_x, move.from.y);

        king.last_move_num = chess->move_num;
        rook.last_move_num = chess->move_num;

        set_piece_at(chess, king, move.to.x, move.to.y);
        set_piece_at(chess, rook, rook_to_x, move.to.y);
    } else if (move.is_en_passant) {
        kgchess_piece_internal_t piece = get_piece_at(chess, move.from.x, move.from.y);
        piece.last_move_num = chess->move_num;
        set_piece_at(chess, empty_piece, move.from.x, move.from.y);
        set_piece_at(chess, piece, move.to.x, move.to.y);
        set_piece_at(chess, empty_piece, move.to.x, move.from.y);
    } else {
        kgchess_piece_internal_t piece = get_piece_at(chess, move.from.x, move.from.y);
        piece.last_move_num = chess->move_num;
        set_piece_at(chess, empty_piece, move.from.x, move.from.y);
        set_piece_at(chess, piece, move.to.x, move.to.y);
        if (piece.type == KGCHESS_PIECE_PAWN) {
            if ((piece.player == KGCHESS_PLAYER_WHITE && move.to.y == 7)
                || (piece.player == KGCHESS_PLAYER_BLACK && move.to.y == 0)) {
                chess->state = KGCHESS_STATE_PROMOTION;
                chess->promotion_pos = move.to;
            }
        }
    }

    chess->move_num++;
    chess->last_move = move;
    if (chess->state == KGCHESS_STATE_MOVE && update_state) {
        chess->current_player = kgchess_get_enemy_player(chess->current_player);
        check_checkmate(chess);
    }
}

static bool is_position_empty(const kgchess_t *chess, int x, int y) {
    kgchess_piece_t piece = kgchess_get_piece_at(chess, x, y);
    return piece.type == KGCHESS_PIECE_NONE;
}

static bool is_player_at_position(const kgchess_t *chess, kgchess_player_t player, int x, int y) {
    kgchess_piece_t piece = kgchess_get_piece_at(chess, x, y);
    return piece.type != KGCHESS_PIECE_NONE && piece.player == player;
}

static kgchess_piece_internal_t piece_make(kgchess_piece_type_t type, kgchess_player_t player) {
    kgchess_piece_internal_t p;
    p.type = type;
    p.player = player;
    p.last_move_num = -1;
    return p;
}

static kgchess_piece_internal_t get_piece_at(const kgchess_t *chess, int x, int y) {
    if (x < 0 || x >= 8 || y < 0 || y >= 8) {
        return piece_make(KGCHESS_PIECE_NONE, KGCHESS_PLAYER_NONE);
    }
    return chess->pieces[x][y];
}

static void set_piece_at(kgchess_t *chess, kgchess_piece_internal_t piece, int x, int y) {
    if (x < 0 || x >= 8 || y < 0 || y >= 8) {
        return;
    }
    chess->pieces[x][y] = piece;
}

static bool is_castling_possible(const kgchess_t *chess, int x, int y, int rook_x) {
    kgchess_piece_internal_t king = get_piece_at(chess, x, y);
    kgchess_piece_internal_t rook = get_piece_at(chess, rook_x, y);

    if (king.type != KGCHESS_PIECE_KING || rook.type != KGCHESS_PIECE_ROOK) {
        return false;
    }
    
    if (king.last_move_num != -1 || rook.last_move_num != -1) {
        return false;
    }

    kgchess_player_t enemy = kgchess_get_enemy_player(king.player);

    if (is_in_check(chess, king.player)) {
        return false;
    }

    // todo: cache attacks?
    if (rook_x == 0) {
        if (!is_position_empty(chess, 1, y) || !is_position_empty(chess, 2, y) || !is_position_empty(chess, 3, y)) {
            return false;
        }
        if (kgchess_is_square_attacked_by_player(chess, 3, y, enemy)) {
            return false;
        }
        return true;
    } else if (rook_x == 7) {
        if (!is_position_empty(chess, 5, y) || !is_position_empty(chess, 6, y)) {
            return false;
        }
        if (kgchess_is_square_attacked_by_player(chess, 5, y, enemy)) {
            return false;
        }
        return true;
    }
    return false;
}

static int get_en_passant(const kgchess_t *chess, int pawn_x, int pawn_y, kgchess_piece_t pawn) {
    if (chess->move_num <= 0) {
        return -1;
    }

    kgchess_move_t last_move = chess->last_move;
    kgchess_piece_t last_move_piece = kgchess_get_piece_at(chess, last_move.to.x, last_move.to.y);
    if (pawn.type != KGCHESS_PIECE_PAWN || last_move_piece.type != KGCHESS_PIECE_PAWN) {
        return -1;
    }

    int enemy_pawn_x = last_move.to.x;
    int enemy_pawn_y = last_move.to.y;

    if (pawn_y != enemy_pawn_y) {
        return -1;
    }

    int last_move_dist = abs(last_move.to.y - last_move.from.y);

    if (last_move_dist != 2) {
        return -1;
    }

    if (pawn_x != (enemy_pawn_x - 1) && pawn_x != (enemy_pawn_x + 1)) {
        return -1;
    }

    return enemy_pawn_x;
}

static bool is_in_check(const kgchess_t *chess, kgchess_player_t player) {
    int king_x = -1;
    int king_y = -1;
    for (int x = 0; x < 8; x++) {
        for (int y = 0; y < 8; y++) {
            kgchess_piece_t piece = kgchess_get_piece_at(chess, x, y);
            if (piece.type == KGCHESS_PIECE_KING && piece.player == player) {
                king_x = x;
                king_y = y;
                break;
            }
        }
    }

    if (king_x == -1 || king_y == -1) {
        return false;
    }

    return kgchess_is_square_attacked_by_player(chess, king_x, king_y, kgchess_get_enemy_player(player));
}

static void check_checkmate(kgchess_t *chess) {
    for (int x = 0; x < 8; x++) {
        for (int y = 0; y < 8; y++) {
            kgchess_piece_t piece = kgchess_get_piece_at(chess, x, y);
            if (piece.type == KGCHESS_PIECE_NONE || piece.player != chess->current_player) {
                continue;
            }
            kgchess_moves_array_t moves = kgchess_get_moves(chess, x, y);
            if (moves.count != 0) {
                return;
            }
        }
    }
    if (is_in_check(chess, chess->current_player)) {
        chess->winner = kgchess_get_enemy_player(chess->current_player);
    }
    chess->state = KGCHESS_STATE_ENDED;
}

static kgchess_piece_t convert_piece(kgchess_piece_internal_t piece) {
    kgchess_piece_t res;
    res.player = piece.player;
    res.type = piece.type;
    return res;
}

static kgchess_moves_array_t get_moves(const kgchess_t *chess, int x, int y, bool add_potential_attacks, bool is_attacks_check) {
    kgchess_piece_t piece = kgchess_get_piece_at(chess, x, y);
    switch (piece.type) {
        case KGCHESS_PIECE_KING:   return get_king_moves(chess, x, y, piece, add_potential_attacks, is_attacks_check);
        case KGCHESS_PIECE_QUEEN:  return get_queen_moves(chess, x, y, piece, add_potential_attacks, is_attacks_check);
        case KGCHESS_PIECE_BISHOP: return get_bishop_moves(chess, x, y, piece, add_potential_attacks, is_attacks_check);
        case KGCHESS_PIECE_KNIGHT: return get_knight_moves(chess, x, y, piece, add_potential_attacks, is_attacks_check);
        case KGCHESS_PIECE_ROOK:   return get_rook_moves(chess, x, y, piece, add_potential_attacks, is_attacks_check);
        case KGCHESS_PIECE_PAWN:   return get_pawn_moves(chess, x, y, piece, add_potential_attacks, is_attacks_check);
        default: break;
    }
    return kgchess_moves_array_make_empty();
}

static kgchess_moves_array_t get_king_moves(const kgchess_t *chess, int x, int y, kgchess_piece_t piece, bool add_potential_attacks, bool is_attacks_check) {
    kgchess_moves_array_t moves = kgchess_moves_array_make_empty();

    add_move_range(chess, &moves, x, y, piece, 0, +1, 1, add_potential_attacks, is_attacks_check);
    add_move_range(chess, &moves, x, y, piece, 0, -1, 1, add_potential_attacks, is_attacks_check);
    add_move_range(chess, &moves, x, y, piece, -1, 0, 1, add_potential_attacks, is_attacks_check);
    add_move_range(chess, &moves, x, y, piece, +1, 0, 1, add_potential_attacks, is_attacks_check);

    add_move_range(chess, &moves, x, y, piece, +1, +1, 1, add_potential_attacks, is_attacks_check);
    add_move_range(chess, &moves, x, y, piece, -1, -1, 1, add_potential_attacks, is_attacks_check);
    add_move_range(chess, &moves, x, y, piece, -1, +1, 1, add_potential_attacks, is_attacks_check);
    add_move_range(chess, &moves, x, y, piece, +1, -1, 1, add_potential_attacks, is_attacks_check);

    if (!is_attacks_check && is_castling_possible(chess, x, y, 0)) {
        kgchess_move_t move = move_make(x, y, 2, y, false, true, false);
        add_move_if_legal(chess, &moves, move, false);
    }
    if (!is_attacks_check && is_castling_possible(chess, x, y, 7)) {
        kgchess_move_t move = move_make(x, y, 6, y, false, true, false);
        add_move_if_legal(chess, &moves, move, false);
    }

    return moves;
}

static kgchess_moves_array_t get_queen_moves(const kgchess_t *chess, int x, int y, kgchess_piece_t piece, bool add_potential_attacks, bool is_attacks_check) {
    kgchess_moves_array_t moves = kgchess_moves_array_make_empty();

    add_move_range(chess, &moves, x, y, piece, 0, +1, 7, add_potential_attacks, is_attacks_check);
    add_move_range(chess, &moves, x, y, piece, 0, -1, 7, add_potential_attacks, is_attacks_check);
    add_move_range(chess, &moves, x, y, piece, -1, 0, 7, add_potential_attacks, is_attacks_check);
    add_move_range(chess, &moves, x, y, piece, +1, 0, 7, add_potential_attacks, is_attacks_check);

    add_move_range(chess, &moves, x, y, piece, +1, +1, 7, add_potential_attacks, is_attacks_check);
    add_move_range(chess, &moves, x, y, piece, -1, -1, 7, add_potential_attacks, is_attacks_check);
    add_move_range(chess, &moves, x, y, piece, -1, +1, 7, add_potential_attacks, is_attacks_check);
    add_move_range(chess, &moves, x, y, piece, +1, -1, 7, add_potential_attacks, is_attacks_check);

    return moves;
}

static kgchess_moves_array_t get_bishop_moves(const kgchess_t *chess, int x, int y, kgchess_piece_t piece, bool add_potential_attacks, bool is_attacks_check) {
    kgchess_moves_array_t moves = kgchess_moves_array_make_empty();

    add_move_range(chess, &moves, x, y, piece, +1, +1, 7, add_potential_attacks, is_attacks_check);
    add_move_range(chess, &moves, x, y, piece, -1, -1, 7, add_potential_attacks, is_attacks_check);
    add_move_range(chess, &moves, x, y, piece, -1, +1, 7, add_potential_attacks, is_attacks_check);
    add_move_range(chess, &moves, x, y, piece, +1, -1, 7, add_potential_attacks, is_attacks_check);

    return moves;
}

static kgchess_moves_array_t get_knight_moves(const kgchess_t *chess, int x, int y, kgchess_piece_t piece, bool add_potential_attacks, bool is_attacks_check) {
    kgchess_moves_array_t moves = kgchess_moves_array_make_empty();
    kgchess_player_t enemy = kgchess_get_enemy_player(piece.player);
    kgchess_pos_t positions_to_check[] = {
        { x + 2, y + 1 },
        { x + 2, y - 1 },
        { x - 2, y + 1 },
        { x - 2, y - 1 },

        { x + 1, y + 2 },
        { x + 1, y - 2 },
        { x - 1, y + 2 },
        { x - 1, y - 2 },
    };

    for (int i = 0; i < ARRAY_LENGTH(positions_to_check); i++) {
        kgchess_pos_t pos = positions_to_check[i];
        kgchess_piece_t temp_piece = kgchess_get_piece_at(chess, pos.x, pos.y);
        if (temp_piece.type == KGCHESS_PIECE_NONE && (!is_attacks_check || add_potential_attacks)) {
            kgchess_move_t move = move_make(x, y, pos.x, pos.y, add_potential_attacks, false, false);
            add_move_if_legal(chess, &moves, move, is_attacks_check);
        } else if (temp_piece.player == enemy) {
            kgchess_move_t move = move_make(x, y, pos.x, pos.y, true, false, false);
            add_move_if_legal(chess, &moves, move, is_attacks_check);
        }
    }
    return moves;
}

static kgchess_moves_array_t get_rook_moves(const kgchess_t *chess, int x, int y, kgchess_piece_t piece, bool add_potential_attacks, bool is_attacks_check) {
    kgchess_moves_array_t moves = kgchess_moves_array_make_empty();

    add_move_range(chess, &moves, x, y, piece, 0, +1, 7, add_potential_attacks, is_attacks_check);
    add_move_range(chess, &moves, x, y, piece, 0, -1, 7, add_potential_attacks, is_attacks_check);
    add_move_range(chess, &moves, x, y, piece, -1, 0, 7, add_potential_attacks, is_attacks_check);
    add_move_range(chess, &moves, x, y, piece, +1, 0, 7, add_potential_attacks, is_attacks_check);

    return moves;
}

static kgchess_moves_array_t get_pawn_moves(const kgchess_t *chess, int x, int y, kgchess_piece_t piece, bool add_potential_attacks, bool is_attacks_check) {
    kgchess_moves_array_t moves = kgchess_moves_array_make_empty();
    int dir = 1;
    int initial_y = 1;
    if (piece.player == KGCHESS_PLAYER_BLACK) {
        dir = -1;
        initial_y = 6;
    }
    if (!is_attacks_check && is_position_empty(chess, x, y + 1 * dir)) {
        kgchess_move_t move = move_make(x, y, x, y + 1 * dir, false, false, false);
        add_move_if_legal(chess, &moves, move, is_attacks_check);
        if (y == initial_y && is_position_empty(chess, x, y + 2 * dir)) {
            kgchess_move_t move = move_make(x, y, x, y + 2 * dir, false, false, false);
            add_move_if_legal(chess, &moves, move, is_attacks_check);
        }
    }
    kgchess_player_t enemy = kgchess_get_enemy_player(piece.player);
    if (add_potential_attacks || is_player_at_position(chess, enemy, x + 1, y + 1 * dir)) {
        kgchess_move_t move = move_make(x, y, x + 1, y + 1 * dir, true, false, false);
        add_move_if_legal(chess, &moves, move, is_attacks_check);
    }
    if (add_potential_attacks || is_player_at_position(chess, enemy, x - 1, y + 1 * dir)) {
        kgchess_move_t move = move_make(x, y, x - 1, y + 1 * dir, true, false, false);
        add_move_if_legal(chess, &moves, move, is_attacks_check);
    }

    if (!add_potential_attacks && !is_attacks_check) {
        int en_passant_x = get_en_passant(chess, x, y, piece);
        if (en_passant_x != -1) {
            kgchess_move_t move = move_make(x, y, en_passant_x, y + dir, true, false, true);
            add_move_if_legal(chess, &moves, move, is_attacks_check);

        }
    }

    return moves;
}

static void add_move_range(const kgchess_t *chess, kgchess_moves_array_t *moves, int x, int y,
                           kgchess_piece_t piece, int dx, int dy,
                           int end, bool add_potential_attacks, bool is_attacks_check)
{
    for (int i = 1; i < (end + 1); i++) {
        kgchess_piece_t temp_piece = kgchess_get_piece_at(chess, x + i * dx, y + i * dy);
        if (temp_piece.type == KGCHESS_PIECE_NONE && (!is_attacks_check || add_potential_attacks)) {
            kgchess_move_t move = move_make(x, y, x + i * dx, y + i * dy, add_potential_attacks, false, false);
            add_move_if_legal(chess, moves, move, is_attacks_check);
        } else if (temp_piece.player == kgchess_get_enemy_player(piece.player)) {
            kgchess_move_t move = move_make(x, y, x + i * dx, y + i * dy, true, false, false);
            add_move_if_legal(chess, moves, move, is_attacks_check);
            break;
        } else if (temp_piece.player == piece.player) {
            break;
        }
    }
}
