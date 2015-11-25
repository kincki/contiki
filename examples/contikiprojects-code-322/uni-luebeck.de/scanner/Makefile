DEFINES=PROJECT_CONF_H=\"project-conf.h\"

# Define the target platform
ifndef TARGET
TARGET=sky
endif

# Give a name to your project 
CONTIKI_PROJECT = scanner_7

# Compile project typing "make"
all: $(CONTIKI_PROJECT)

# Upload project typing "make upload"
upload: $(CONTIKI_PROJECT).upload

# Define additional source files to be compiled (if any)
CONTIKI_TARGET_SOURCEFILES += interferer_settings.c

# ContikiProjects: including the makefile
include ../../Makefile.projects

# Normal Contiki
#CONTIKI = ../..
#include $(CONTIKI)/Makefile.include

# Include Testbed
#include /home/cboano/testbed-dir/Makefile.include

# Including Math library
CUSTOM_RULE_LINK=1
%.$(TARGET): %.co $(PROJECT_OBJECTFILES) $(PROJECT_LIBRARIES) contiki-$(TARGET).a
	$(LD) $(LDFLAGS) $(TARGET_STARTFILES) ${filter-out %.a,$^} ${filter %.a,$^} $(TARGET_LIBFILES) -o $@ -lm

