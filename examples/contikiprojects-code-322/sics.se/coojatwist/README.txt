1) Add the plugin to your tools/cooja/apps dirctory
2) Compile it with ant
3) Add it to the Cooja external projects by starting cooja and going to "Settings -> COOJA projects"
4) Create a new simulation
5) Start the plugin via "Plugins -> TWIST testbed"
6) Generate TWIST motes "Cooja actions -> Generate motes"
7) Connect the TIWST motes to the COOJA log listener "Cooja actions -> Connect motes"
8) Upload your binary* to TWIST "Twist actions -> Upload binary"
9) Set simulation speed to 100% and start the simulation. Serial output from the TWIST motes should appear in the log listener.

*If you're using Contiki, read the file contikisources/README.txt
