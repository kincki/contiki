CONTIKI_PROJECT = rest-example
ifndef TARGET
TARGET=sky
endif
all: $(CONTIKI_PROJECT)

APPS = logger simplexml webservices
CFLAGS = -DWITH_UIP=1 -I. -DWITH_NULLMAC=1  #-DPERSISTENT_LOG -DLOG_ENABLED=2 

#Set Contiki Home here
CONTIKI = ../contiki-2.x
include $(CONTIKI)/Makefile.include
