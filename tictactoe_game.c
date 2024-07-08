#include "tictactoe_game.h"
#include "tictactoe_internal.h"
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/printk.h>

int tictactoe_game_init(ttt_game_t *game)
{
	game->next_turn = 'X';
	game->winner = 0;
	for (int x = 0; x < 3; x++) {
		for (int y = 0; y < 3; y++) {
			game->board[x][y] = ' ';
		}
	}

	return 0;
}

// returns 0 if there is no winner yet, -1 if it's a tie, and the respective player if they win
char tictactoe_game_check_winner(ttt_game_t *game)
{
	char winner;

	// check columns
	for (int x = 0; x < 3; x++) {
		winner = game->board[x][0];
		if (winner != ' ') {
			for (int y = 1; y < 3; y++) {
				if (game->board[x][y] != winner) {
					winner = 0;
					break;
				}
			}
			if (winner != 0)
				return winner;
		}
	}

	// check rows
	for (int y = 0; y < 3; y++) {
		winner = game->board[0][y];
		if (winner != ' ') {
			for (int x = 1; x < 3; x++) {
				if (game->board[x][y] != winner) {
					winner = 0;
					break;
				}
			}
			if (winner != 0)
				return winner;
		}
	}

	// check diagonal top-left <-> bottom-right
	winner = game->board[0][0];
	if (winner != ' ') {
		for (int i = 0; i < 3; i++) {
			if (game->board[i][i] != winner) {
				winner = 0;
				break;
			}
		}
		if (winner != 0)
			return winner;
	}

	// check diagonal bottom-left <-> top-right
	winner = game->board[2][0];
	if (winner != ' ') {
		for (int i = 0; i < 3; i++) {
			if (game->board[2 - i][i] != winner) {
				winner = 0;
				break;
			}
		}
		if (winner != 0)
			return winner;
	}

	// check for tie
	winner = -1;
	for (int x = 0; x < 3; x++) {
		for (int y = 0; y < 3; y++) {
			if (game->board[x][y] == ' ') {
				winner = 0;
				break;
			}
		}
	}

	return winner;
}

int tictactoe_game_make_turn(ttt_game_t *game, size_t x, size_t y)
{
	if (game->board[x][y] != ' ') {
		pr_notice("position already occupied\n");
		return -1;
	}

	game->board[x][y] = game->next_turn;
	game->winner = tictactoe_game_check_winner(game);

	if (game->next_turn == 'X')
		game->next_turn = 'O';
	else
		game->next_turn = 'X';

	return 0;
}

int tictactoe_game_snprint(ttt_game_t *game, char *buf, size_t count)
{
	int pos;
	int ret;

	pos = 0;

	ret = snprintf(buf + pos, count - pos,
		       "#####\n#%.3s#\n#%.3s#\n#%.3s#\n#####\n\n",
		       game->board[0], game->board[1], game->board[2]);
	if (ret < 0)
		goto fail;
	pos += ret;

	if (game->winner == 0) {
		ret = snprintf(buf + pos, count - pos, "It's %c's turn!\n",
			       game->next_turn);
	} else if (game->winner == -1) {
		ret = snprintf(buf + pos, count - pos, "It's a tie!\n");
	} else {
		ret = snprintf(buf + pos, count - pos, "Player %c won!\n",
			       game->winner);
	}
	if (ret < 0)
		goto fail;
	pos += ret;

	return pos;

fail:
	tictactoe_error("snprintf failed\n");
	return -1;
}
