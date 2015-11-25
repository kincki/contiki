ifndef CONTIKI
  ${error CONTIKI not defined! You must specify where Contiki resides}
endif

CONTIKI_PROJECT = breadbox64
all: $(CONTIKI_PROJECT)

PROJECTDIRS = ../../include ../../lib ../../cpu/6502

PROJECT_SOURCEFILES = base64.c escape.c twitter_resolve.c twitter_post.c parse.c twitter_timeline.c input.c strings.c

TARGET = win32

DEFINES = WITH_GUI

CONTIKI_WITH_IPV4 = 1

include $(CONTIKI)/Makefile.include
