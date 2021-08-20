# AmslerGUI
It is a Application GUI for Amsler Tribometer

Some screenshoots:

![Konfiguracja](https://obrazki.elektroda.pl/2090611200_1602948316.png "Konfiguracja")

![Wykres](https://obrazki.elektroda.pl/8905062900_1602948317.png "Wykres")

How to upload the code:

1. upload the bootloader
2. prefer the platformIO to upload the code via USB with these config:
   [env:genericSTM32F103CB]
   platform = ststm32
   board = genericSTM32F103CB
   framework = arduino
   ;upload_flags = -c set CPUTAPID 0x2ba01477
   board_build.core = maple
   upload_protocol = dfu
   upload_speed = 115200
   upload_port = COM5 
