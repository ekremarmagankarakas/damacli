# damacli

Turkish draughts (dama) for the terminal, in C++20. Men move and capture forward
and sideways; kings fly orthogonally. Mandatory longest capture.

## Build & run

```
make build
./build/damacli
```

Launches the full-screen TUI by default. Arrows move the cursor, Enter
selects / lands, Esc cancels, type a move or command and Enter to submit.

## In-game commands

```
a3a4          # quiet move
a3xa5         # single capture
a3xa5xc5      # capture chain (longest is mandatory)
undo  reset  history  resign  help  quit
```

## CLI flags

```
--tui              # force TUI (default)
--no-tui, --cli    # CLI prompt instead
--text             # ASCII view (CLI only; default is Unicode)
--engine DEPTH     # minimax depth (default 3)
--no-engine        # two-player, no AI
--play-black       # engine plays White
--configure        # open config editor, save, and exit
--help, -h
```

## Config

`$XDG_CONFIG_HOME/damacli/config.ini` (falls back to `~/.config/damacli/`).
Edit with `damacli --configure`. CLI flags override the file for that run.

## Make targets

| Target         | Action                       |
|----------------|------------------------------|
| `make build`   | Configure + compile          |
| `make run`     | Build + launch               |
| `make test`    | Run unit tests (doctest)     |
| `make format`  | clang-format in place        |
| `make check`   | Verify formatting            |
| `make clean`   | Remove build dir             |

Requires CMake ≥ 3.20, a C++20 compiler, and `clang-format`.
