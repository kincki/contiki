all: ipsec-example border-router

ifndef TARGET
TARGET=sky
endif

CONTIKI = /home/user/Work/devs/contiki-2.7
CONTIKI_TOOLS = /home/user/contiki-tools

UIP_CONF_IPV6=1

CFLAGS += -DPROJECT_CONF_H=\"project-conf.h\"
CFLAGS += -DLOCAL_PLATFORM_CONF_H=1
PROJECT_SOURCEFILES += slip-bridge.c ipsec.c aes-ctr.c aes-xcbc-mac.c cc2420-aes-mapping.c miracl-aes.c
SMALL = 1

#include $(CONTIKI)/Makefile.include
include Makefile.include

$(CONTIKI)/tools/tunslip6:	$(CONTIKI)/tools/tunslip6.c
	(cd $(CONTIKI)/tools && $(MAKE) tunslip6)

connect-router:	$(CONTIKI)/tools/tunslip6
	sudo $(CONTIKI_TOOLS)/tunslip6 aaaa::1/64

connect-router-cooja: $(CONTIKI)/tools/tunslip6
	sudo $(CONTIKI_TOOLS)/tunslip6 -a 127.0.0.1 aaaa::1/64

realclean: clean
	rm -rf obj_* symbols.c symbols.h *.sky *.log
