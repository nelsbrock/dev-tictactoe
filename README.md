# /dev/tictactoe

A Linux driver for a character device which can be used to play
[Tic-tac-toe](https://en.wikipedia.org/wiki/Tic-tac-toe).

## Usage

*Disclaimer:* I make no guarantees about the soundness of the driver code.
Use with caution (i.e. only in a virtual machine).

### Building

Clone this repository, `cd` to it, then run `make`.

### Loading

After building the module, run `insmod tictactoe.ko` as root to load it.

### Using the device

After loading the module, player X can write grid coordinates
(ranging from 1 to 3, format `XY`) to `/dev/tictactoe` to make their first move:

```console
# echo 12 > /dev/tictactoe
```

After player X has made his move, player O can do the same:

```console
# echo 22 > /dev/tictactoe
```

To inspect the current state of the game, simply read from `/dev/tictactoe`:

```console
# cat /dev/tictactoe
```

```
#####
# X #
# O #
#   #
#####

It's X's turn!
```

Alternatively, you can `watch` the file in a separate terminal for automatic
updating:

```console
# watch -n 0.5 cat /dev/tictactoe
```

To start a new game, write `reset` to `/dev/tictactoe`:

```console
# echo reset > /dev/tictactoe
```

### Unloading

Run `rmmod tictactoe` as root to unload the module.
