# CAN bus onboarding

An easy first project for the control team: an introduction to the STM32 and
the CAN bus. Three simulated RoboMaster Development Board Type A (STM32F427)
boards share one CAN bus in the [Renode](https://renode.io) simulator, so no
hardware is needed.

All the setup is done for you (clocks, pins, CAN1 bit rate, serial output).
**You write four small functions** in `firmware/Core/Src/main.c` using the
STM32 HAL, and an automatic test tells you when each one works.

**Start here: [docs/CAN-Bus-Onboarding.pdf](docs/CAN-Bus-Onboarding.pdf)**, the step-by-step guide.
It covers CAN basics, setup, a tour of every file, the HAL functions you'll
use, the four tasks with hints, challenges, and moving to real boards.

## Quick start

Easiest: open this repository in **GitHub Codespaces** (green **Code** button →
**Codespaces** → **Create codespace on main**), wait for the setup to finish, then:

```bash
make sim
```

Type `quit` to stop. On your own machine you need `arm-none-eabi-gcc` and
Renode 1.17; see chapter 3 of the guide.

| Command      | What it does                                                   |
|--------------|----------------------------------------------------------------|
| `make`       | Builds `build/node1.elf`, `node2.elf`, `node3.elf`             |
| `make sim`   | Runs all three boards on one CAN bus in Renode                 |
| `make test`  | Checks your tasks (`sim/can_bus.robot`); all fail on the starter |

## The tasks

| Task | Function in `main.c`                    | You learn                                |
|------|-----------------------------------------|------------------------------------------|
| 1    | `show_on_leds()`                        | GPIO, active-low LEDs, bit masks         |
| 2    | `can_start()`                           | CAN filters, starting the controller     |
| 3    | `send_heartbeat()`                      | Building and sending a CAN frame         |
| 4    | `HAL_CAN_RxFifo0MsgPendingCallback()`   | Receiving with an interrupt              |

When all four are done, each board sends a heartbeat (`0x101`, `0x102`,
`0x103` every 500/600/700 ms), prints what it receives on USART6 and shows the
received counter in binary on its 8 LEDs (PG1–PG8).

## Layout

```
firmware/   CubeMX-style STM32 project (edit Core/Src/main.c)
sim/        Renode board description, 3-board bus script, Robot test
docs/       the guide (guide.html is the source of the PDF)
```

`firmware/Drivers/` contains ST's STM32F4 HAL and CMSIS, unchanged, under
their own licenses (see the LICENSE files in those folders).

## For mentors

The `solution` branch has the finished `main.c` (all tests pass there, and CI
checks it on every push) plus `SOLUTIONS.md` with answers to the challenges.
On this repository's `main` branch, CI only builds, since the starter's tests
are meant to fail.
