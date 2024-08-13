#include "tictactoe_game.h"
#include "tictactoe_internal.h"
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/printk.h>

/*
 * Initializes a new game instance.
 */
int tictactoe_game_init(ttt_game_t *game)
{
	game->next_turn = 'X';
	game->winner = 0;
	for (size_t x = 0; x < 3; x++) {
		for (size_t y = 0; y < 3; y++) {
			game->board[x][y] = ' ';
		}
	}

	return 0;
}

/*
 * Steps over the board, starting from position (start_x, start_y) and ending
 * at the board's boundaries, checking if all cells are occupied by the same
 * player.
 *
 * start_x and start_y must lie within the board's boundaries.
 *
 * Returns the respective player, or '\x00' if a cell isn't occupied.
 */
static char tictactoe_game_check_streak(ttt_game_t *game, size_t start_x,
					size_t start_y, ssize_t step_x,
					ssize_t step_y)
{
	char winner;
	size_t x;
	size_t y;

	winner = game->board[start_x][start_y];
	x = start_x + step_x;
	y = start_y + step_y;

	if (winner == ' ')
		return 0;

	while (x >= 0 && x < 3 && y >= 0 && y < 3) {
		if (game->board[x][y] != winner)
			return 0;
		x += step_x;
		y += step_y;
	}

	return winner;
}

/*
 * Checks for a winner with the current board configuration.
 *
 * Returns
 *   '\x00' if the game has not ended,
 *   '\xFF' if the game is tied, or
 *   the winner of the game.
 */
static char tictactoe_game_check_winner(ttt_game_t *game)
{
	char winner;

	// check columns
	for (size_t x = 0; x < 3; x++) {
		if ((winner = tictactoe_game_check_streak(game, x, 0, 0, 1)))
			return winner;
	}

	// check rows
	for (size_t y = 0; y < 3; y++) {
		if ((winner = tictactoe_game_check_streak(game, 0, y, 1, 0)))
			return winner;
	}

	// check diagonal top-left <-> bottom-right
	if ((winner = tictactoe_game_check_streak(game, 0, 0, 1, 1)))
		return winner;

	// check diagonal bottom-left <-> top-right
	if ((winner = tictactoe_game_check_streak(game, 0, 2, 1, -1)))
		return winner;

	// check for tie
	winner = '\xFF';
	for (size_t x = 0; x < 3; x++) {
		for (size_t y = 0; y < 3; y++) {
			if (game->board[x][y] == ' ') {
				winner = 0;
				break;
			}
		}
	}

	return winner;
}

/*
 * Occupies the given board position for the player whose turn it is.
 *
 * This function does *not* check whether there is a winner first.
 * x and y must lie within the board's boundaries.
 *
 * Returns
 *   -1 if the given position is already occupied, or
 *   0 on success.
 */
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

/*
 * Prints a human-readable visualization of the game state to buf.
 */
int tictactoe_game_snprint(ttt_game_t *game, char *buf, size_t count)
{
	size_t pos;
	int ret;

	pos = 0;

	ret = snprintf(buf + pos, count - pos,
		       "#####\n#%.3s#\n#%.3s#\n#%.3s#\n#####\n\n",
		       game->board[0], game->board[1], game->board[2]);
	if (ret < 0)
		goto fail;
	pos += ret;

	if (!game->winner) {
		ret = snprintf(buf + pos, count - pos, "It's %c's turn!\n",
			       game->next_turn);
	} else if (game->winner == '\xFF') {
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
