# Give a name to your project 
CONTIKI_PROJECT = incremental_interferer
all: $(CONTIKI_PROJECT)

# Define the target platform
ifndef TARGET
TARGET=sky
endif

# Define additional source files to be compiled (if any)
CONTIKI_TARGET_SOURCEFILES += interferer_settings.c

%.class: %.java
	javac $(basename $<).java

run: GetNoise.class	
	java -classpath . GetNoise

# Including the makefile
include ../../Makefile.projects
