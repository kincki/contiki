#ifeq ($(TARGET),sky)
#else ifeq ($(TARGET),native)
#CONTIKI_PROJECT = philo-pt mt-cep
#endif

CONTIKI_PROJECT = philo-mt  philo-mt-deadlock philo-mt-deadlock-free
all: $(CONTIKI_PROJECT)

CONTIKI = ../../..
ifeq ($(TARGET),z1)
CONTIKI_WITH_IPV6 = 1
else
CONTIKI_WITH_RIME = 1
endif

include $(CONTIKI)/Makefile.include
