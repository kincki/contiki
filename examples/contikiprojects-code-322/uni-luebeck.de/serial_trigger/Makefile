# Give a name to your project 
CONTIKI_PROJECT = trigger_through_serial
all: $(CONTIKI_PROJECT)

# Define the target platform
ifndef TARGET
TARGET=sky
endif

# Define additional source files to be compiled (if any)
# CONTIKI_TARGET_SOURCEFILES += filename.c

%.class: %.java
	javac $(basename $<).java

bind: BindMotes.class	
	java -classpath . BindMotes


# Including the makefile
include ../../Makefile.projects
