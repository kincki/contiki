CONTIKI_PROJECT = udp-client udp-server cep-sensors 
all: $(CONTIKI_PROJECT)
APPS=servreg-hack


CONTIKI = ../../..
#CONTIKI_WITH_RIME = 1

ifdef SERVER_REPLY
CFLAGS+=-DSERVER_REPLY=$(SERVER_REPLY)
endif
ifdef PERIOD
CFLAGS+=-DPERIOD=$(PERIOD)
endif


include $(CONTIKI)/Makefile.include
