#all: broadcast-example broadcast-aNumber broadcast-aNumber-recv
all:  broadcast-aNumber broadcast-aNumber-recv

APPS=servreg-hack
CONTIKI=../../..

CFLAGS += -DPROJECT_CONF_H=\"project-conf.h\"

# automatically build RESTful resources
REST_RESOURCES_DIR = ./resources
ifndef TARGET
REST_RESOURCES_FILES = $(notdir $(shell find $(REST_RESOURCES_DIR) -name '*.c'))
else
ifeq ($(TARGET), native)
REST_RESOURCES_FILES = $(notdir $(shell find $(REST_RESOURCES_DIR) -name '*.c'))
else
REST_RESOURCES_FILES = $(notdir $(shell find $(REST_RESOURCES_DIR) -name '*.c' ! -name 'res-plugtest*'))
endif
endif

PROJECTDIRS += $(REST_RESOURCES_DIR)
PROJECT_SOURCEFILES += $(REST_RESOURCES_FILES)

# REST Engine shall use Erbium CoAP implementation
APPS += er-coap
APPS += rest-engine

CONTIKI_WITH_IPV6 = 1
include $(CONTIKI)/Makefile.include
