/*
 * Copyright (c) 2010, Swedish Institute of Computer Science.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the Institute nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *
 */

import java.io.File;
import java.util.ArrayList;

import org.apache.log4j.Logger;

import se.sics.cooja.MoteType;
import se.sics.cooja.RadioPacket;
import se.sics.cooja.Simulation;
import se.sics.cooja.motes.AbstractApplicationMote;
import se.sics.cooja.util.StringUtils;

public class TwistReplayMote extends AbstractApplicationMote {
  private static Logger logger = Logger.getLogger(TwistReplayMote.class);

  //private static final String TWIST_LOG_FILE = "twist_trace.gz";
  private static ArrayList<LogOutput> allLogOutput = null;
  private static long startTime = Long.MIN_VALUE;

  private boolean started = false;
  private ArrayList<LogOutput> myLogOutput = new ArrayList<LogOutput>();
  private LogOutput scheduled = null;

  long excludeTimesBefore = -1*Simulation.MILLISECOND+Simulation.MILLISECOND;
  long excludeTimesAfter = -1*Simulation.MILLISECOND;

  class LogOutput {
    boolean hasMote = false;
    int id;
    String log;
    long time;
    int counter = -1;
    public LogOutput(int id, String log, long time) {
      this.id = id;
      this.log = log;
      this.time = time;
    }
  }

  public TwistReplayMote() {
    super();
  }

  public TwistReplayMote(MoteType moteType, Simulation simulation) {
    super(moteType, simulation);
  }

  private void readLogFile() {
    allLogOutput = new ArrayList<LogOutput>();

    /* Read file contents */
    String logFilename = getSimulation().getGUI().currentConfigFile.getName();
    logFilename = logFilename.replace(".csc", ".gz");
    File logFile = new File(getSimulation().getGUI().currentConfigFile.getParentFile(), logFilename);
    if (!logFile.exists()) {
      log("# WARNING: Could not open " + logFile.getName() + " (" + logFile.getParentFile().getAbsolutePath() + ")");
      return;
    }
    log("# Opening " + logFile.getName() + " (" + logFile.getParentFile().getAbsolutePath() + ")");
    String log = StringUtils.loadFromFile(logFile);
    if (log == null || log.isEmpty()) {
      log("# WARNING: Could not read " + logFile.getName() + " (" + logFile.getParentFile().getAbsolutePath() + ")");
      return;
    }

    for (String l: log.split("\n")) {
      try {
        createLogEntry(l);
      } catch (Exception e) {
        logger.warn("Error parsing log output: " + l);
      }
    }
    log("# Read " + allLogOutput.size() + " log messages from " + logFile.getName());
  }

  private void createLogEntry(String l) throws Exception {
    if (l.startsWith("#")) {
      return;
    }
    String[] arr = l.split("[ \t]");
    if (arr.length != 3) {
      logger.warn("Bad input: " + l);
      return;
    }

    int id = Integer.parseInt(arr[1]);

    /* Create log entry */
    String outputHex = arr[2];
    String output = convert(outputHex);
    double time = Double.parseDouble(arr[0]);
    long timeLong = (long) (Simulation.MILLISECOND*1000L*time);

    LogOutput lo = new LogOutput(id, output, timeLong);
    if (startTime == Long.MIN_VALUE || startTime > timeLong - Simulation.MILLISECOND*1000L) {
      startTime = timeLong - Simulation.MILLISECOND*1000L;
    }

    /* XXX Parse log counter */
    try {
      arr = output.split(" ");
      if (arr[0].equals("S") ||
          arr[0].equals("X") ||
          arr[0].equals("C") ||
          arr[0].equals("W")) {
        int c = Integer.parseInt(arr[1],16);
        lo.counter = c;
      }
    } catch (Exception e) {
    }
    allLogOutput.add(lo);
  }

  private int lastCounter = Integer.MIN_VALUE;
  private void parseLog() {
    if (allLogOutput == null) {
      readLogFile();
    }

    /* Parse log */
    boolean foundEntry = false;
    for (LogOutput lo: allLogOutput) {
      if (lo.id != getID()) {
        continue;
      }

      lo.time -= startTime;
      if (excludeTimesBefore >= 0 && lo.time < excludeTimesBefore) {
        lo.hasMote = true;
        continue;
      }
      if (excludeTimesAfter >= 0 && lo.time > excludeTimesAfter) {
        lo.hasMote = true;
        continue;
      }

      boolean isDuplicate = false;
      foundEntry = true;

      /* XXX Verify counters */
      if (lo.counter < 0) { 
        /* No counter */
      } else if (lastCounter == Integer.MIN_VALUE) {
        lastCounter = lo.counter;
      } else if (lastCounter + 1 == lo.counter) {
        /* Great! No lost messages... */
      } else if (lastCounter == lo.counter) {
        /* Duplicate! That's ok... */
        isDuplicate = true;
      } else if (lastCounter + 1 == lo.counter - 1) {
        /* Missed a single message */
        String log = "# ERROR: Missing a single message: " + Integer.toHexString(lastCounter+1);
        myLogOutput.add(new LogOutput(lo.id, log, lo.time));
      } else if (lastCounter + 1 < lo.counter - 1) {
        String log = "# ERROR (MAJOR): Missing several messages: " + Integer.toHexString(lastCounter+1) + "<->" + Integer.toHexString(lo.counter-1);
        myLogOutput.add(new LogOutput(lo.id, log, lo.time));
      } else {
        logger.debug("# ERROR Potential node reboot at time: " + (lo.time) + "? (ID:" + lo.id + "). " + "Log output was: " + lo.log);
        String log = "# ERROR: Did node reboot?";
        myLogOutput.add(new LogOutput(lo.id, log, lo.time));
      }
      if (lo.counter >= 0) {
        lastCounter = lo.counter;
      }

      /* Create log entry */
      lo.hasMote = true;
      if (isDuplicate) {
        lo.log = "# DUP: " + lo.log;
      }
      myLogOutput.add(lo);
    }

    if (!foundEntry) {
      log("# WARNING: No log messages found for ID " + getID());
    } else {
      //log("# Parsed " + myLogOutput.size() + " log outputs (" + unaccountedLogs() + ")");
    }
  }

  private String unaccountedLogs() {
    int unaccounted = 0;
    String exampleID = null;
    for (LogOutput lo: allLogOutput) {
      if (!lo.hasMote) {
        unaccounted++;
        if (exampleID == null) {
          exampleID = "" + lo.id;
        }
      }
    }

    if (unaccounted == 0) {
      return "all accounted for";
    }
    return unaccounted + " left, next ID " + exampleID;
  }

  private static String convert(String s) {
//    final int LEN = 86;
//    if (s.length() != LEN) {
//      logger.warn("Parsing failed: " + s);
//      return("# Parsing failed (" + s.length() + " != " + LEN + "): " + s);
//    }

    if (s.length() % 2 != 0) {
      logger.warn("Parsing failed, uneven length: " + s);
      return("# Parsing failed, uneven length: " + s);
    }

    StringBuilder converted = new StringBuilder();
    s = s.substring(16); /* strip header */

    while (s.length() > 0) {
      char c = (char) Integer.parseInt(s.substring(0, 2), 16);
      s = s.substring(2);
      if (c == 0) {
        continue;
      }
      converted.append(c);
    }

    return converted.toString().trim();
  }

  public void execute(long time) {
    /*System.out.println(this + ": execute(" + time + ")");*/

    if (!started) {
      parseLog();
      started = true;
    }

    /* Scheduled log output */
    if (scheduled != null) {
      log(scheduled.log);
      if (myLogOutput.isEmpty()) {
        //log("# No more messages from ID " + getID());
      }
    }

    /* Schedule next */
    if (myLogOutput != null && myLogOutput.size() > 0) {
      scheduled = myLogOutput.remove(0);
      scheduleNextWakeup(scheduled.time);
    }
  }

  public String toString() {
    return "TwistReplay " + getID();
  }

  public void receivedPacket(RadioPacket p) {
  }
  public void sentPacket(RadioPacket p) {
  }
}

