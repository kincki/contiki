ifeq ($(TARGET),sky)
CONTIKI_PROJECT = philo-mt  philo-mt-deadlock philo-mt-deadlock-free
else ifeq ($(TARGET),native)
CONTIKI_PROJECT = philo-pt mt-cep
endif

all: $(CONTIKI_PROJECT)

CONTIKI = ../../..
CONTIKI_WITH_RIME = 1
include $(CONTIKI)/Makefile.include
