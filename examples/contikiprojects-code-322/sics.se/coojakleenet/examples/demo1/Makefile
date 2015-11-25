CONTIKI_PROJECT = client router server
all: $(CONTIKI_PROJECT)

TARGET=coojakleenet

ifndef CONTIKI
NO_MAKEFILE_INCLUDE=1
include ../../../../Makefile.projects
NO_MAKEFILE_INCLUDE=
CONTIKI:=../../$(CONTIKI)
endif

WITH_UIP=1
include ../../../../Makefile.projects
