#ifndef __TICTACTOE_GAME_H__
#define __TICTACTOE_GAME_H__

#include <linux/types.h>

typedef struct tictactoe_game {
	char next_turn;
	char winner;
	char board[3][3];
} ttt_game_t;

int tictactoe_game_init(ttt_game_t *game);
int tictactoe_game_make_turn(ttt_game_t *game, size_t x, size_t y);
int tictactoe_game_snprint(ttt_game_t *game, char *buf, size_t len);

#endif // __TICTACTOE_GAME_H__
