CONTIKI_PROJECT = telnet-server

###stm32w port only compiles with UIP_CONF_IPV6=1 now
UIP_CONF_IPV6=1
PROJECTDIRS = $(CONTIKI)/apps/shell $(CONTIKI)/platform/mb851/apps \
			$(CONTIKI)/apps/telnetd
PROJECT_SOURCEFILES = shell-blink.c shell-power.c shell-ps.c \
				shell-reboot.c shell-sensors.c shell-text.c shell-time.c \
				shell.c telnetd.c

all: $(CONTIKI_PROJECT)

CONTIKI = ../..
include $(CONTIKI)/Makefile.include
