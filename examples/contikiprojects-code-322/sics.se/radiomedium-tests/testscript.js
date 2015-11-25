TIMEOUT(1200000000);
WAIT_UNTIL(msg.equals("TEST START"));

REPS = 200;
DISTANCE_STEP = 100;
MAX_DISTANCE = 5000;
PERIOD = 1000; /* Synchronize w Transmitter.java */

mote.getSimulation().getMoteWithID(1).getInterfaces().getPosition().setCoordinates(0,0,0);
rxMote = mote.getSimulation().getMoteWithID(2);
rxMotePos = rxMote.getInterfaces().getPosition();
rxDist = 0;
rxMotePos.setCoordinates(rxDist,0,0);
intMote = mote.getSimulation().getMoteWithID(3);
intMote.getInterfaces().getPosition().setCoordinates(MAX_DISTANCE,0,0);
noiseMote = mote.getSimulation().getMoteWithID(4);
noiseMote.getInterfaces().getPosition().setCoordinates(MAX_DISTANCE/2,0,0);

/////////////////////////////////////////////////////


GENERATE_MSG(PERIOD/2, "move");
YIELD_THEN_WAIT_UNTIL(msg.equals("move"));

/* #1: Simple PRR test */
log.log("#1: Simple PRR test\n");
round = 0;
prr = new Array();
while (round++ < REPS) {
  //log.log("# round " + round + "\n");

  rxDist = 0;
  rxMotePos.setCoordinates(rxDist,0,0);
  GENERATE_MSG(PERIOD, "move");
  while (rxDist <= MAX_DISTANCE) {
    YIELD();

    if (msg.equals("RX")) {
      if (prr[rxDist/DISTANCE_STEP] == undefined) prr[rxDist/DISTANCE_STEP] = 0; 
      prr[rxDist/DISTANCE_STEP]++;
      //log.log("# prr[rxDist/DISTANCE_STEP] " + prr[rxDist/DISTANCE_STEP] + "\n");
    }
    
    if (msg.equals("move")) {
      msg = "";
      rxDist += DISTANCE_STEP;
      if (rxDist <= MAX_DISTANCE) {
        rxMotePos.setCoordinates(rxDist,0,0);
        GENERATE_MSG(PERIOD, "move");
      }
    }
  }
}

/* #1: Simple PRR test: print results */
rxDist = 0;
while (rxDist <= MAX_DISTANCE) {
  if (prr[rxDist/DISTANCE_STEP] == undefined) prr[rxDist/DISTANCE_STEP] = 0; 
  log.log(rxDist + " " + (1.0*prr[rxDist/DISTANCE_STEP]/REPS) + "\n");
  rxDist += DISTANCE_STEP;
}
log.log("\n");
prr_1 = prr;

/////////////////////////////////////////////////////


GENERATE_MSG(PERIOD, "move");
YIELD_THEN_WAIT_UNTIL(msg.equals("move"));

intMote.setActive(true, 500*1000 - 5*1000);

/* #2: PRR test with early interferer */
log.log("#2: PRR test with early interferer\n");
round = 0;
prr = new Array();
while (round++ < REPS) {
  //log.log("# round " + round + "\n");

  rxDist = 0;
  rxMotePos.setCoordinates(rxDist,0,0);
  GENERATE_MSG(PERIOD, "move");
  while (rxDist <= MAX_DISTANCE) {
    YIELD();

    if (msg.equals("RX")) {
      if (prr[rxDist/DISTANCE_STEP] == undefined) prr[rxDist/DISTANCE_STEP] = 0; 
      prr[rxDist/DISTANCE_STEP]++;
      //log.log("# prr[rxDist/DISTANCE_STEP] " + prr[rxDist/DISTANCE_STEP] + "\n");
    }
    
    if (msg.equals("move")) {
      msg = "";
      rxDist += DISTANCE_STEP;
      if (rxDist <= MAX_DISTANCE) {
        rxMotePos.setCoordinates(rxDist,0,0);
        GENERATE_MSG(PERIOD, "move");
      }
    }
  }
}
intMote.setActive(false, 0);

/* #2: PRR test with early interferer: print results */
rxDist = 0;
while (rxDist <= MAX_DISTANCE) {
  if (prr[rxDist/DISTANCE_STEP] == undefined) prr[rxDist/DISTANCE_STEP] = 0; 
  log.log(rxDist + " " + (1.0*prr[rxDist/DISTANCE_STEP]/REPS) + "\n");
  rxDist += DISTANCE_STEP;
}
log.log("\n");
prr_2 = prr;


/////////////////////////////////////////////////////


GENERATE_MSG(PERIOD, "move");
YIELD_THEN_WAIT_UNTIL(msg.equals("move"));

intMote.setActive(true, 500*1000 + 5*1000);

/* #3: PRR test with late interferer */
log.log("#3: PRR test with late interferer\n");
round = 0;
prr = new Array();
while (round++ < REPS) {
  //log.log("# round " + round + "\n");

  rxDist = 0;
  rxMotePos.setCoordinates(rxDist,0,0);
  GENERATE_MSG(PERIOD, "move");
  while (rxDist <= MAX_DISTANCE) {
    YIELD();

    if (msg.equals("RX")) {
      if (prr[rxDist/DISTANCE_STEP] == undefined) prr[rxDist/DISTANCE_STEP] = 0; 
      prr[rxDist/DISTANCE_STEP]++;
      //log.log("# prr[rxDist/DISTANCE_STEP] " + prr[rxDist/DISTANCE_STEP] + "\n");
    }
    
    if (msg.equals("move")) {
      msg = "";
      rxDist += DISTANCE_STEP;
      if (rxDist <= MAX_DISTANCE) {
        rxMotePos.setCoordinates(rxDist,0,0);
        GENERATE_MSG(PERIOD, "move");
      }
    }
  }
}
intMote.setActive(false, 0);

/* #3: PRR test with late interferer: print results */
rxDist = 0;
while (rxDist <= MAX_DISTANCE) {
  if (prr[rxDist/DISTANCE_STEP] == undefined) prr[rxDist/DISTANCE_STEP] = 0; 
  log.log(rxDist + " " + (1.0*prr[rxDist/DISTANCE_STEP]/REPS) + "\n");
  rxDist += DISTANCE_STEP;
}
log.log("\n");
prr_3 = prr;


/////////////////////////////////////////////////////


GENERATE_MSG(PERIOD, "move");
YIELD_THEN_WAIT_UNTIL(msg.equals("move"));

noiseMote.setActive(true);

/* #4: PRR test with external noise */
log.log("#4: PRR test with external noise\n");
round = 0;
prr = new Array();
while (round++ < REPS) {
  //log.log("# round " + round + "\n");

  rxDist = 0;
  rxMotePos.setCoordinates(rxDist,0,0);
  GENERATE_MSG(PERIOD, "move");
  while (rxDist <= MAX_DISTANCE) {
    YIELD();

    if (msg.equals("RX")) {
      if (prr[rxDist/DISTANCE_STEP] == undefined) prr[rxDist/DISTANCE_STEP] = 0; 
      prr[rxDist/DISTANCE_STEP]++;
      //log.log("# prr[rxDist/DISTANCE_STEP] " + prr[rxDist/DISTANCE_STEP] + "\n");
    }
    
    if (msg.equals("move")) {
      msg = "";
      rxDist += DISTANCE_STEP;
      if (rxDist <= MAX_DISTANCE) {
        rxMotePos.setCoordinates(rxDist,0,0);
        GENERATE_MSG(PERIOD, "move");
      }
    }
  }
}
noiseMote.setActive(false);

/* #4: PRR test with external noise: print results */
rxDist = 0;
while (rxDist <= MAX_DISTANCE) {
  if (prr[rxDist/DISTANCE_STEP] == undefined) prr[rxDist/DISTANCE_STEP] = 0; 
  log.log(rxDist + " " + (1.0*prr[rxDist/DISTANCE_STEP]/REPS) + "\n");
  rxDist += DISTANCE_STEP;
}
log.log("\n");
prr_4 = prr;


/* Generate google charts address */
url = "chart?cht=lc&";
url += "chs=500x250&chco=ff0000,00ff00,0000ff,00ffff&";
url += "chdl=No interference|Ongoing tx|Colliding tx|External noise&";
url += "chxt=x,y&";

/* Distances */
url += "chl=";
rxDist = 0;
skip = 0;
skip_reset = 0.1*MAX_DISTANCE/DISTANCE_STEP;
while (rxDist <= MAX_DISTANCE) {
  if (skip <= 0) {
    url += rxDist + "m";
    skip = skip_reset;
  } else {
    skip--;
  }
  if (rxDist < MAX_DISTANCE) url += "|";
  rxDist += DISTANCE_STEP;
}
url += "&";

/* Data */
url += "chd=t:";
rxDist = 0;
while (rxDist <= MAX_DISTANCE) {
  url += (100.0*prr_1[rxDist/DISTANCE_STEP]/REPS);
  if (rxDist < MAX_DISTANCE) url += ",";
  rxDist += DISTANCE_STEP;
}
url += "|";
rxDist = 0;
while (rxDist <= MAX_DISTANCE) {
  url += (100.0*prr_2[rxDist/DISTANCE_STEP]/REPS);
  if (rxDist < MAX_DISTANCE) url += ",";
  rxDist += DISTANCE_STEP;
}
url += "|";
rxDist = 0;
while (rxDist <= MAX_DISTANCE) {
  url += (100.0*prr_3[rxDist/DISTANCE_STEP]/REPS);
  if (rxDist < MAX_DISTANCE) url += ",";
  rxDist += DISTANCE_STEP;
}
url += "|";
rxDist = 0;
while (rxDist <= MAX_DISTANCE) {
  url += (100.0*prr_4[rxDist/DISTANCE_STEP]/REPS);
  if (rxDist < MAX_DISTANCE) url += ",";
  rxDist += DISTANCE_STEP;
}

url = new java.lang.String(url);
url = url.replace(" ","%20");
url = url.replace("|","%7C");
url = "http:\/\/chart.googleapis.com/" + url;

log.log("Google charts:\n");
log.log(url + "\n");


/* Start browser */
//new java.lang.Runtime.getRuntime().exec("rundll32 url.dll,FileProtocolHandler " + url);

/* Done */
log.testOK();
