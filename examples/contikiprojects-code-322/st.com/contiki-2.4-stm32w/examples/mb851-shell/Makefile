CONTIKI_PROJECT = mb851-shell

###stm32w port only compiles with UIP_CONF_IPV6=1 now
UIP_CONF_IPV6=1
PROJECTDIRS = $(CONTIKI)/apps/serial-shell \
			$(CONTIKI)/apps/shell $(CONTIKI)/platform/mb851/apps
PROJECT_SOURCEFILES = serial-shell.c shell-blink.c shell-power.c shell-ps.c \
					shell-reboot.c shell-sensors.c shell-text.c shell-time.c \
					shell.c

all: $(CONTIKI_PROJECT)

CONTIKI = ../..
include $(CONTIKI)/Makefile.include
