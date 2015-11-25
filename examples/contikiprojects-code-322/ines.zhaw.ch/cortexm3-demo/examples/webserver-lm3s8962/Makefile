CONTIKI_PROJECT = webserver-lm3s8962

CONTIKI_TARGET_SOURCEFILES += 
CONTIKI_SOURCEFILES 			 += $(CONTIKI_TARGET_SOURCEFILES)

APPS=webserver
TARGET = lm3s8962-evk


all: $(CONTIKI_PROJECT).bin

#for windows machines using LM Flash
prog: $(CONTIKI_PROJECT).bin
	lmflash -r $<

CONTIKI = ../..
include $(CONTIKI)/Makefile.include
