*** Settings ***
Documentation       Automatic check for the CAN bus onboarding project.
...                 Run with:  make test

*** Variables ***
${UART}             sysbus.usart6

*** Keywords ***
Start The Bus
    Execute Script              ${CURDIR}/can_bus.resc
    ${n1}=                      Create Terminal Tester    ${UART}    machine=node1
    ${n2}=                      Create Terminal Tester    ${UART}    machine=node2
    ${n3}=                      Create Terminal Tester    ${UART}    machine=node3
    Start Emulation
    RETURN                      ${n1}    ${n2}    ${n3}

*** Test Cases ***
Every Node Hears The Other Two
    ${n1}    ${n2}    ${n3}=    Start The Bus

    Wait For Line On Uart       [node1] rx id=0x102    testerId=${n1}
    Wait For Line On Uart       [node1] rx id=0x103    testerId=${n1}

    Wait For Line On Uart       [node2] rx id=0x101    testerId=${n2}
    Wait For Line On Uart       [node2] rx id=0x103    testerId=${n2}

    Wait For Line On Uart       [node3] rx id=0x101    testerId=${n3}
    Wait For Line On Uart       [node3] rx id=0x102    testerId=${n3}

No Node Hears Itself
    ${n1}    ${n2}    ${n3}=    Start The Bus

    Should Not Be On Uart       [node1] rx id=0x101    testerId=${n1}    timeout=2
    Should Not Be On Uart       [node2] rx id=0x102    testerId=${n2}    timeout=2
    Should Not Be On Uart       [node3] rx id=0x103    testerId=${n3}    timeout=2

LEDs Show The Received Counter In Binary
    ${n1}    ${n2}    ${n3}=    Start The Bus
    ${led_a}=                   Create LED Tester    sysbus.gpioPortG.led_a    machine=node2
    ${led_b}=                   Create LED Tester    sysbus.gpioPortG.led_b    machine=node2

    # node1's second heartbeat carries counter = 1 (binary 00000001)
    Wait For Line On Uart       [node2] rx id=0x101 dlc=2 data: 01 01    testerId=${n2}
    Assert LED State            true     testerId=${led_a}    timeout=0.1
    Assert LED State            false    testerId=${led_b}    timeout=0.1
