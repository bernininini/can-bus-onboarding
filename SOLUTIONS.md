# Solutions (mentors only)

This is the `solution` branch. `firmware/Core/Src/main.c` here has Tasks 1–4
filled in, and `make test` passes. To see exactly what a student has to write:

```bash
git diff main solution -- firmware/Core/Src/main.c
```

Below are answers to the three challenges in chapter 7 of the guide. Each one
starts from the finished Task 4 code and was built and run in Renode; the
expected output in the guide comes from these. The challenges change the bus
traffic, so some `make test` checks fail afterwards. That's expected.

## Challenge 1: send a 16-bit number

In `send_heartbeat()`:

```c
header.DLC = 4;
...
uint16_t value = NODE_ID * 1000 + counter;
uint8_t data[4] = { NODE_ID, counter, value >> 8, value & 0xFF };
```

`print_tx()` already prints all `DLC` bytes. In `print_rx()`, just before the
final `printf("\r\n");`:

```c
if (header->DLC >= 4)
{
  uint16_t value = (data[2] << 8) | data[3];
  printf("  -> value=%u", value);
}
```

Backwards byte order would turn 1000 (`0x03E8`) into `0xE803` = 59395.

## Challenge 2: filters

In `can_start()`:

```c
filter.FilterIdHigh = 0x101 << 5;
filter.FilterIdLow = 0x0000;
filter.FilterMaskIdHigh = 0x7FF << 5;
filter.FilterMaskIdLow = 0x0000;
```

To accept both 0x102 and 0x103: ID `0x102 << 5`, mask `0x7FE << 5` (bit 0 is
"don't care").

## Challenge 3: a DJI-style motor command

Prototype, in `USER CODE BEGIN PFP`:

```c
static void send_motor_command(int16_t m1, int16_t m2, int16_t m3, int16_t m4);
```

The function, next to `send_heartbeat()`:

```c
static void send_motor_command(int16_t m1, int16_t m2, int16_t m3, int16_t m4)
{
  CAN_TxHeaderTypeDef header = {0};
  header.StdId = 0x200;
  header.IDE = CAN_ID_STD;
  header.RTR = CAN_RTR_DATA;
  header.DLC = 8;
  uint8_t data[8] = { m1 >> 8, m1, m2 >> 8, m2, m3 >> 8, m3, m4 >> 8, m4 };
  uint32_t mailbox;
  if (HAL_CAN_AddTxMessage(&hcan1, &header, data, &mailbox) == HAL_OK)
  {
    print_tx(&header, data);
  }
}
```

In the main loop, after `send_heartbeat(counter++);`:

```c
if (NODE_ID == 1)
{
  send_motor_command(1000, -1000, 0, 0);
}
```

And after `print_rx(&f.header, f.data);`:

```c
if (f.header.StdId == 0x200 && NODE_ID != 1)
{
  int slot = NODE_ID - 2;   /* node2 = motor 1 = bytes 0-1 */
  int16_t current = (int16_t)((f.data[2 * slot] << 8) | f.data[2 * slot + 1]);
  printf("[node%d] my current command = %d\r\n", NODE_ID, current);
}
```

−1000 in 16-bit two's complement is 65536 − 1000 = 64536 = `0xFC18`. The LEDs
show `data[1]` of every frame, and in a 0x200 frame that's the low byte of
motor 1's command, `0xE8` = `11101000`.
