# CAN bus onboarding: builds the same firmware three times, once per node.
#
#   make        build build/node1.elf, build/node2.elf, build/node3.elf
#   make sim    build, then run all three nodes on one CAN bus in Renode
#   make test   build, then run the automatic check (sim/can_bus.robot)
#   make clean  delete build/ and test reports
#
# The only difference between the three builds is -DNODE_ID=1/2/3.

NODES   = 1 2 3
PREFIX ?= arm-none-eabi-
CC      = $(PREFIX)gcc
SIZE    = $(PREFIX)size
RENODE ?= renode
RENODE_TEST ?= renode-test
RENODE_FLAGS ?= --console --disable-gui

FW   = firmware
SRCS = $(wildcard $(FW)/Core/Src/*.c) $(wildcard $(FW)/Drivers/STM32F4xx_HAL_Driver/Src/*.c)
ASMS = $(FW)/Core/Startup/startup_stm32f427xx.s
INCS = -I$(FW)/Core/Inc \
       -I$(FW)/Drivers/STM32F4xx_HAL_Driver/Inc \
       -I$(FW)/Drivers/STM32F4xx_HAL_Driver/Inc/Legacy \
       -I$(FW)/Drivers/CMSIS/Device/ST/STM32F4xx/Include \
       -I$(FW)/Drivers/CMSIS/Include

MCU     = -mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard
CFLAGS  = $(MCU) -std=gnu11 -Og -g -Wall -ffunction-sections -fdata-sections \
          -DUSE_HAL_DRIVER -DSTM32F427xx -DNODE_ID=$(NODE_ID) $(INCS)
LDFLAGS = $(MCU) -T$(FW)/STM32F427IIHX_FLASH.ld -specs=nano.specs -specs=nosys.specs \
          -Wl,--gc-sections -Wl,--no-warn-rwx-segments -Wl,-Map=$(OUT)/node$(NODE_ID).map

OUT    = build
OBJDIR = $(OUT)/node$(NODE_ID)
OBJS   = $(SRCS:%.c=$(OBJDIR)/%.o) $(ASMS:%.s=$(OBJDIR)/%.o)

all:
	@for n in $(NODES); do $(MAKE) --no-print-directory node NODE_ID=$$n || exit 1; done

node: $(OUT)/node$(NODE_ID).elf

$(OUT)/node$(NODE_ID).elf: $(OBJS)
	@echo "LD  $@"
	@$(CC) $(OBJS) $(LDFLAGS) -o $@
	@$(SIZE) $@

$(OBJDIR)/%.o: %.c
	@mkdir -p $(dir $@)
	@echo "CC  $<  (node $(NODE_ID))"
	@$(CC) $(CFLAGS) -MMD -MP -c $< -o $@

$(OBJDIR)/%.o: %.s
	@mkdir -p $(dir $@)
	@echo "AS  $<"
	@$(CC) $(MCU) -c $< -o $@

sim: all
	$(RENODE) $(RENODE_FLAGS) sim/can_bus.resc -e start

test: all
	$(RENODE_TEST) sim/can_bus.robot

clean:
	rm -rf $(OUT) log.html report.html robot_output.xml snapshots logs

-include $(OBJS:.o=.d)

.PHONY: all node sim test clean
