*** Settings ***
Documentation       Automatic checks for the CAN bus onboarding tasks.
...                 Run with:  make test
...                 Each test is named after the task it checks. In the starter
...                 project they all fail; they pass one by one as you finish
...                 the tasks.

*** Variables ***
${UART}             sysbus.usart6
${CAN1_MCR}         0x40006400
${CAN1_FA1R}        0x4000661C

*** Keywords ***
Load The Bus
    Execute Script              ${CURDIR}/can_bus.resc

Register Bits Should Be
    [Arguments]    ${machine}    ${address}    ${mask}    ${expected}    ${message}
    Execute Command             mach set "${machine}"
    ${value}=                   Execute Command    sysbus ReadDoubleWord ${address}
    ${value}=                   Convert To Integer    ${value.strip()}
    ${bits}=                    Evaluate    ${value} & ${mask}
    Should Be Equal As Integers    ${bits}    ${expected}    msg=${message}    values=False

*** Test Cases ***
Task 1 - LEDs Show NODE_ID At Boot
    [Documentation]             node3 shows 3 = 0b00000011: LED_A and LED_B on, LED_C off.
    Load The Bus
    ${n3}=                      Create Terminal Tester    ${UART}    machine=node3    defaultPauseEmulation=true
    ${led_a}=                   Create LED Tester    sysbus.gpioPortG.led_a    machine=node3
    ${led_b}=                   Create LED Tester    sysbus.gpioPortG.led_b    machine=node3
    ${led_c}=                   Create LED Tester    sysbus.gpioPortG.led_c    machine=node3
    Start Emulation

    Wait For Line On Uart       [node3] up:    testerId=${n3}
    Assert LED State            true     testerId=${led_a}
    Assert LED State            true     testerId=${led_b}
    Assert LED State            false    testerId=${led_c}

Task 2 - CAN1 Is Started With A Filter
    [Documentation]             After can_start(), CAN1 has left initialization mode and filter bank 0 is active.
    Load The Bus
    ${n1}=                      Create Terminal Tester    ${UART}    machine=node1    defaultPauseEmulation=true
    Start Emulation

    Wait For Line On Uart       [node1] up:    testerId=${n1}
    Register Bits Should Be     node1    ${CAN1_MCR}     0x1    0
    ...                         CAN1 is still in initialization mode. Did HAL_CAN_Start() run and return HAL_OK?
    Register Bits Should Be     node1    ${CAN1_FA1R}    0x1    1
    ...                         Filter bank 0 is not active. Check FilterBank, FilterActivation and HAL_CAN_ConfigFilter().

Task 3 - Each Node Sends Its Heartbeat
    [Documentation]             Frames 0x101, 0x102 and 0x103 appear on the bus, each 2 bytes long, starting with the node's number.
    Load The Bus
    Create CAN Tester           canHub    defaultTimeout=2
    Start Emulation

    ${data}=                    Wait For Frame With Id    0x101
    Length Should Be            ${data}    2
    Should Be Equal As Integers    ${data[0]}    1
    ${data}=                    Wait For Frame With Id    0x102
    Length Should Be            ${data}    2
    Should Be Equal As Integers    ${data[0]}    2
    ${data}=                    Wait For Frame With Id    0x103
    Length Should Be            ${data}    2
    Should Be Equal As Integers    ${data[0]}    3

Task 4 - Every Node Hears The Other Two But Never Itself
    Load The Bus
    ${n1}=                      Create Terminal Tester    ${UART}    machine=node1
    ${n2}=                      Create Terminal Tester    ${UART}    machine=node2
    ${n3}=                      Create Terminal Tester    ${UART}    machine=node3
    Start Emulation

    Wait For Line On Uart       [node1] rx id=0x102    testerId=${n1}
    Wait For Line On Uart       [node1] rx id=0x103    testerId=${n1}
    Wait For Line On Uart       [node2] rx id=0x101    testerId=${n2}
    Wait For Line On Uart       [node2] rx id=0x103    testerId=${n2}
    Wait For Line On Uart       [node3] rx id=0x101    testerId=${n3}
    Wait For Line On Uart       [node3] rx id=0x102    testerId=${n3}

    Should Not Be On Uart       [node1] rx id=0x101    testerId=${n1}    timeout=1
    Should Not Be On Uart       [node2] rx id=0x102    testerId=${n2}    timeout=1
    Should Not Be On Uart       [node3] rx id=0x103    testerId=${n3}    timeout=1

Task 4 - LEDs Show The Received Counter In Binary
    [Documentation]             After node2 receives counter 1 from node1, LED_A is on and LED_B (its own NODE_ID bit) is off.
    Load The Bus
    ${n2}=                      Create Terminal Tester    ${UART}    machine=node2
    ${led_a}=                   Create LED Tester    sysbus.gpioPortG.led_a    machine=node2
    ${led_b}=                   Create LED Tester    sysbus.gpioPortG.led_b    machine=node2
    Start Emulation

    # node1's second heartbeat carries counter = 1 (binary 00000001)
    Wait For Line On Uart       [node2] rx id=0x101 dlc=2 data: 01 01    testerId=${n2}
    Assert LED State            true     testerId=${led_a}    timeout=0.1
    Assert LED State            false    testerId=${led_b}    timeout=0.1
