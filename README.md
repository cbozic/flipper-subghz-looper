# Sub-GHz Looper

A [Flipper Zero](https://flipperzero.one/) application that replays previously captured
Sub-GHz signals on a repeating timer. Point it at one or more `.sub` captures, pick how
often to re-send them, and it will rebroadcast them in a loop until you stop it.

> **Responsible use.** Transmitting on Sub-GHz frequencies is regulated in most regions,
> and replaying captured signals can affect real devices (gates, remotes, sensors). Only
> transmit signals you are authorized to send, on frequencies and at duty cycles that are
> legal where you are. You are responsible for how you use this app.

## What it does

- Reads `.sub` captures from `/ext/subghz` — the folder the Flipper's built-in **Sub-GHz**
  app saves recordings to.
- Transmits a selection of those captures, in order, on a timer you configure.
- Uses the Flipper's **internal CC1101** radio. Both keyed-protocol captures (e.g. Princeton,
  came, etc.) and **RAW** captures are supported. Captures saved with an unsupported or
  custom preset are skipped and reported as failures rather than transmitted.

## Using the app on the device

The app installs under **Apps → Sub-GHz → Sub-GHz Looper**. The main menu has five entries:

1. **Select Files** — a checklist of every `.sub` file directly under `/ext/subghz`.
   - **Up/Down** move the cursor, **OK** toggles a file's `[x]`, **Back** saves the
     selection and returns to the menu.
2. **Interval** — how long to wait between one broadcast cycle and the next.
   - Set an integer **Interval** value (1–255) and a **Unit** of Seconds, Minutes, or Hours.
3. **LED on Broadcast** — toggle **On**/**Off** (default **Off**). When on, the Flipper's LED
   blinks red for the duration of each broadcast cycle, off the rest of the time.
4. **Run** — starts the loop **immediately**:
   - It broadcasts every selected file once, right away.
   - Then it counts down the interval (`Next in: Xs`) and rebroadcasts when it reaches zero,
     repeating until you leave.
   - **OK** toggles pause/resume, **Back** stops the loop and returns to the menu.
   - The circle in the top-right is an activity indicator: **filled** while a signal is
     actively broadcasting, **hollow** while idle/waiting. After each cycle the screen shows
     `Last: N sent, M FAILED` so you can see whether anything failed to transmit.
5. **About** — a short description of the app.

**Persistence:** your file selection is saved when you leave the Select Files screen, and the
interval and LED-on-broadcast settings are saved when you exit the app. All are restored on
the next launch.

## Requirements

- A Flipper Zero running official firmware (or a compatible fork) — the app builds against
  the released SDK API.
- One or more `.sub` captures in `/ext/subghz` (record them with the stock Sub-GHz app first).

## Development environment

Build tooling is [`ufbt`](https://github.com/flipperdevices/flipperzero-ufbt) (the "micro
Flipper Build Tool"). It is installed into a **local Python virtual environment** at
`.venv/`, **not** globally — this keeps the toolchain scoped to this project.

> Please do **not** `pip install ufbt` (or anything else) globally for this repo. Always go
> through `.venv/`.

### First-time setup

```sh
python3 -m venv .venv
.venv/bin/pip install --upgrade pip
.venv/bin/pip install ufbt
```

The first build downloads and caches the ARM toolchain and Flipper SDK under `~/.ufbt`
(outside the repo, shared across projects). Only the `ufbt` tool itself lives in `.venv/`.

If you prefer not to prefix every command, activate the venv once per shell session:

```sh
source .venv/bin/activate   # then just run `ufbt ...`
```

The rest of this document uses the explicit `.venv/bin/ufbt` form.

### Common tasks

| Task | Command |
| --- | --- |
| **Build** the app (produces `dist/subghz_looper.fap`) | `.venv/bin/ufbt` |
| **Build + install + launch** on a connected Flipper (USB) | `.venv/bin/ufbt launch` |
| Open a **Flipper CLI** session over USB (useful for `log`) | `.venv/bin/ufbt cli` |
| Generate **VS Code** build/debug config for this app | `.venv/bin/ufbt vscode_dist` |
| **Update** the cached SDK/toolchain | `.venv/bin/ufbt update` |
| Lint / format the C/C++ sources | `.venv/bin/ufbt lint` / `.venv/bin/ufbt format` |
| Flash device firmware over USB (rarely needed) | `.venv/bin/ufbt flash_usb` |

The typical inner loop while developing is just:

```sh
.venv/bin/ufbt launch     # rebuilds only what changed, then runs it on the device
```

### Debugging on-device

Runtime logs (including crash reports) are easiest to read over the Flipper CLI:

```sh
.venv/bin/ufbt cli
# then, at the Flipper prompt:
log debug
```

Reproduce the issue on the device and the log stream will show `FURI_LOG_*` output and any
crash/assert details. Press `Ctrl+C` to stop the log stream.

## Build output

- `dist/subghz_looper.fap` — the installable app package.
- `dist/debug/` — the debug ELF (symbols, for debugging).

`.venv/`, `.vscode/`, and `dist/` are git-ignored.

## Project layout

```
application.fam        App manifest (id, name, entry point, category, icon)
app.cpp / app.hpp      App shell: menu, view dispatcher, shared radio, teardown
about/                 About screen
files/                 "Select Files" checklist view (scans /ext/subghz)
interval/              "Interval" settings view (value + unit)
led/                   "LED on Broadcast" settings view (on/off toggle)
run/                   "Run" view + background worker thread that drives the loop
subghz_tx/             Radio driver: loads a .sub file and transmits it (internal CC1101)
easy_flipper/          Small helpers for allocating GUI views (from the starter template)
font/                  Font helpers (from the starter template)
.devcontainer/         Alternative containerized dev environment (see below)
```

## Alternative: Dev Container

A `.devcontainer/` is included as a containerized way to get the same toolchain (it installs
Python + `ufbt` in the container). The local venv is the primary/default workflow; the dev
container is there if you'd rather build inside a container or on a machine where you don't
want to install the toolchain directly.
