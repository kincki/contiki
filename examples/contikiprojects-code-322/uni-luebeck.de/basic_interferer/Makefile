DEFINES=PROJECT_CONF_H=\"project-conf.h\"

# Define the target platform
ifndef TARGET
TARGET=sky
endif

# Give a name to your project 
CONTIKI_PROJECT = interferer_example_3

# Compile project typing "make"
all: interferer_example_1 interferer_example_2 interferer_example_3 interferer_example_4 \
     interferer_example_5 interferer_example_6 interferer_example_7     

# Upload project typing "make upload"
upload: $(CONTIKI_PROJECT).upload

# Define additional source files to be compiled (if any)
CONTIKI_TARGET_SOURCEFILES += settings_cc2420_rssi.c
CONTIKI_TARGET_SOURCEFILES += settings_cc2420_interferer.c

# ContikiProjects: including the makefile
include ../../Makefile.projects

# Including Math library
CUSTOM_RULE_LINK=1
%.$(TARGET): %.co $(PROJECT_OBJECTFILES) $(PROJECT_LIBRARIES) contiki-$(TARGET).a
	$(LD) $(LDFLAGS) $(TARGET_STARTFILES) ${filter-out %.a,$^} ${filter %.a,$^} $(TARGET_LIBFILES) -o $@ -lm
