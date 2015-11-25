DEFINES=PROJECT_CONF_H=\"project-conf.h\"

# Define the target platform
ifndef TARGET
TARGET=sky
endif

# Give a name to your project 
CONTIKI = ../../../../contiki-2.x/
CONTIKI_PROJECT = sensys_jammer

# Compile project typing "make"
all: $(CONTIKI_PROJECT)

# Upload project typing "make upload"
upload: $(CONTIKI_PROJECT).upload

%.class: %.java
	javac $(basename $<).java
	
viewinterferer: ViewInterferer.class
	make login | java ViewInterferer

# Including the makefile
include ../../../Makefile.projects
