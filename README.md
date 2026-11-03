# CAN bus onboarding

A small first project for the control team: three simulated RoboMaster
Development Board Type A (STM32F427) boards talk over one CAN bus in the
[Renode](https://renode.io) simulator. You write real STM32 HAL code, the same
code you would flash onto a board, with no hardware needed.

**Start here: [docs/CAN-Bus-Onboarding.pdf](docs/CAN-Bus-Onboarding.pdf)**, the full step-by-step guide
(CAN basics, setup, a code walkthrough, four exercises with solutions, and
moving to real boards).

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
| `make test`  | Runs the automatic check in `sim/can_bus.robot`                |

## What each board does

| Board | Sends CAN ID | Every  | Data          |
|-------|--------------|--------|---------------|
| node1 | `0x101`      | 500 ms | `01`, counter |
| node2 | `0x102`      | 600 ms | `02`, counter |
| node3 | `0x103`      | 700 ms | `03`, counter |

Every board prints the frames it receives on USART6 and shows the counter in
binary on its 8 LEDs (PG1–PG8).

## Layout

```
firmware/   CubeMX-style STM32 project (edit Core/Src/main.c)
sim/        Renode board description, 3-board bus script, Robot test
docs/       the guide (guide.html is the source of the PDF)
```

`firmware/Drivers/` contains ST's STM32F4 HAL and CMSIS, unchanged, under
their own licenses (see the LICENSE files in those folders).
