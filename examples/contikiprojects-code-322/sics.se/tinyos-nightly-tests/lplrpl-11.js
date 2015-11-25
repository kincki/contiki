timeout = 10*60*1000*1000;

sinkid = 11;
rx = new Array();
while (time < timeout) {
  if (id == 11 && msg.contains(" RX ")) {
    parts = msg.split(" ");
    sink = parts[2];
    from = parts[3];
    seq = parts[4];
    tot = parts[5];
    log.log((time/1000) + ": rpl rx from " + from + ", seq=" + seq + "\n");
    
    if (rx[from] == undefined) {
      rx[from] = 0;
    }
    rx[from]++;
  }

  YIELD();
}

timeline = sim.getGUI().getPlugin("se.sics.cooja.plugins.TimeLine");
if (timeline != null) {
  log.log(timeline.extractStatistics(false, false, true, true) + "\n");
}

log.log("\nSUMMARY:\n");
hasZeroRx = false;
for (i=1; i < sim.getMotes().length; i++) {
  if (rx[i] == undefined) {
    rx[i] = 0;
  }    
  if (rx[i] == 0 && i != sinkid) {
    hasZeroRx = true;
  }
  log.log("\t#rx from " + i + ": " + rx[i] + "\n");
}

if (hasZeroRx) {
  log.testFailed();
} else {
  log.testOK();
}
