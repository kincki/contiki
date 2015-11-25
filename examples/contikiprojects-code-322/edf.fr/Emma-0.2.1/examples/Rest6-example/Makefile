FORMAT = Emma-Rest6
PORT = /dev/ttyUSB0

#avr-emmalampe
#avr-meshnectics

all:
	make -f Makefile.$(FORMAT) TARGET=avr-meshnectics $(FORMAT).elf
	avr-objcopy -O ihex -R .eeprom -R .fuse -R .signature $(FORMAT).elf $(FORMAT).hex
	avr-size -C --mcu=atmega1281 $(FORMAT).elf
	avr-objcopy -O srec -R .eeprom -R .fuse -R .signature $(FORMAT).elf $(FORMAT).srec

#	meshprog -f $(FORMAT).srec -t $(PORT)
#	avrdude -c jtag2 -p m1281 -P usb -U flash:w:"Emma-Rest6.hex"
clean:
	make -f Makefile.$(FORMAT) TARGET=avr-zigbit clean
	rm -rf obj_*
	rm *.elf
	rm *.hex
	rm symbols.*
	find ./* -name "#*#" -exec echo "[del] " {} \; -exec rm -f {} \; -o -name "*~" -exec echo "[del] " {} \; -exec rm -f {} \;

