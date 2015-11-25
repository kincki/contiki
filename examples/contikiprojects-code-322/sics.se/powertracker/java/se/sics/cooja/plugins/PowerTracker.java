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
 * $Id: PowerTracker.java,v 1.21 2009/12/14 13:25:04 fros4943 Exp $
 */

package se.sics.cooja.plugins;

import java.awt.BorderLayout;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import java.awt.event.MouseAdapter;
import java.awt.event.MouseEvent;
import java.text.DecimalFormat;
import java.text.Format;
import java.util.ArrayList;
import java.util.Collection;
import java.util.Observable;
import java.util.Observer;

import javax.swing.BorderFactory;
import javax.swing.Box;
import javax.swing.JLabel;
import javax.swing.JToolTip;
import javax.swing.Popup;
import javax.swing.PopupFactory;
import javax.swing.Timer;

import org.apache.log4j.Logger;
import org.jdom.Element;

import se.sics.cooja.ClassDescription;
import se.sics.cooja.GUI;
import se.sics.cooja.Mote;
import se.sics.cooja.PluginType;
import se.sics.cooja.Simulation;
import se.sics.cooja.VisPlugin;
import se.sics.cooja.SimEventCentral.MoteCountListener;
import se.sics.cooja.interfaces.Radio;
import se.sics.cooja.interfaces.Radio.RadioEvent;

/**
 * Tracks radio events to sum up transmission, reception, and radio on times.
 * This plugin can be run without visualization, i.e. from a Contiki test.
 * 
 * @author Fredrik Osterlind, Adam Dunkels
 */
@ClassDescription("PowerTracker")
@PluginType(PluginType.SIM_PLUGIN)
public class PowerTracker extends VisPlugin {
  private static final long serialVersionUID = -4428872314369777569L;
  private static final int POWERTRACKER_UPDATE_INTERVAL = 100; /* ms */
  private static Logger logger = Logger.getLogger(PowerTracker.class);

  private Simulation simulation;
  private MoteCountListener moteCountListener;

  private ArrayList<MoteTracker> moteTrackers = new ArrayList<MoteTracker>();

  private JLabel label;
  private JLabel maxLabel;

  public PowerTracker(final Simulation simulation, final GUI gui) {
    super("PowerTracker", gui, false);
    this.simulation = simulation;

    /* Automatically add/delete motes */
    simulation.getEventCentral().addMoteCountListener(moteCountListener = new MoteCountListener() {
      public void moteWasAdded(Mote mote) {
        addMote(mote);
      }
      public void moteWasRemoved(Mote mote) {
        removeMote(mote);
      }
    });
    for (Mote m: simulation.getMotes()) {
      addMote(m);
    }

    if (!GUI.isVisualized()) {
      return;
    }
    
    Box mainBox = Box.createVerticalBox();
    mainBox.setBorder(BorderFactory.createEmptyBorder(10, 10, 10, 10));

    label = new JLabel();
    mainBox.add(label);
    maxLabel = new JLabel();
    mainBox.add(maxLabel);

    mainBox.addMouseListener(new MouseAdapter() {
      private Popup popUpToolTip = null;
      public void mousePressed(MouseEvent e) {
        super.mousePressed(e);
        if (popUpToolTip != null) {
          popUpToolTip.hide();
          popUpToolTip = null;
        }

        JToolTip t = createToolTip();
        t.setTipText(radioStatistics(true, true, false, true));
        if (t.getTipText() == null || t.getTipText().equals("")) {
          return;
        }
        popUpToolTip = PopupFactory.getSharedInstance().getPopup(
            PowerTracker.this, t, e.getXOnScreen(), e.getYOnScreen());
        popUpToolTip.show();
      }
      public void mouseReleased(MouseEvent e) {
        super.mouseReleased(e);
        if (popUpToolTip != null) {
          popUpToolTip.hide();
          popUpToolTip = null;
        }
      }
    });

    this.getContentPane().add(BorderLayout.CENTER, mainBox);
    setSize(290, 100);

    repaintTimer.start();
  }

  public String radioStatistics() {
    return radioStatistics(true, true, false, false);
  }

  public String radioStatistics(boolean radioHW, boolean radioRXTX, boolean shortAverage, boolean html) {
    StringBuilder sb = new StringBuilder();
    long duration = 0;
    long radioOn = 0;
    long radioTx = 0;
    long radioRx = 0;
    long radioInterfered = 0;

    String procent = " %";
    if (html) {
      sb.append("<html>");
      procent = "% ";
    }

    MoteTracker max = null;
    for (MoteTracker t: moteTrackers) {
      duration += (simulation.getSimulationTime() - t.startTime);
      radioOn += t.radioOn;
      radioTx += t.radioTx;
      radioRx += t.radioRx;
      radioInterfered += t.radioInterfered;
      
      if (max == null || t.radioOn > max.radioOn) {
        max = t;
      }

      if (!shortAverage) {
        sb.append(t.toString(radioHW, radioRXTX, html));
      }
    }

    if (duration == 0) {
      return "No data";
    }

    double radioOnAverage = (double) radioOn / duration;
    double radioTxAverage = (double) radioTx / duration;
    double radioRxAverage = (double) radioRx / duration;
    double radioInterferedAverage = (double) radioInterfered / duration;

    if (!shortAverage) {
      Format format = new DecimalFormat("####.000");
      String id = "AVG ";
      if (html) {
        sb.append("<b>Average</b><br>");
        id = "";
      }
      if (radioHW) {
        sb.append(id + "ON " + 
            (html?"":(radioOn + " us ")) +
            format.format(100.0*radioOnAverage) + procent);
        if (!html) {
          sb.append("\n");
        }
      }
      if (radioRXTX) {
        sb.append(id + "TX " + 
            (html?"":(radioTx + " us ")) +
            format.format(100.0*radioTxAverage) + procent);
        if (!html) {
          sb.append("\n");
        }
        sb.append(id + "RX " + 
            (html?"":(radioRx + " us ")) +
            format.format(100.0*radioRxAverage) + procent);
        sb.append(id + "INT " + 
            (html?"":(radioInterfered + " us ")) +
            format.format(100.0*radioInterferedAverage) + procent);
        if (!html) {
          sb.append("\n");
        }
      }
      if (html) {
        sb.append("<br>");
      }
    } else {
      Format format = new DecimalFormat("#0.00");
      sb.append(
          "ON " + format.format(100.0*radioOnAverage) + "% " +
          "TX " + format.format(100.0*radioTxAverage) + "% " +
          "RX " + format.format(100.0*radioRxAverage) + "% "
      );
      sb.append(
          ";Max ON " + format.format(100.0*max.radioOn/(simulation.getSimulationTime() - max.startTime)) + "% @ " +
          "ID " + max.mote.getID()
      );
    }

    if (html) {
      sb.append("</html>");
    }
    return sb.toString();
  }

  abstract class MoteTracker implements Observer {
    private Format format = new DecimalFormat("###0.0000");

    long radioOn = 0;
    long radioTx = 0;
    long radioRx = 0;
    long radioInterfered = 0;

    Radio radio;
    Mote mote;
    long startTime;

    public MoteTracker(Mote mote, Radio radio, long startTime) {
      this.mote = mote;
      this.radio = radio;
      this.startTime = startTime;

      radio.addObserver(this);
    }

    protected void accumulateRadioOn(long t) {
      radioOn += t;
    }
    protected void accumulateRadioTx(long t) {
      radioTx += t;
    }
    protected void accumulateRadioRx(long t) {
      radioRx += t;
    }
    protected void accumulateRadioIntefered(long t) {
      radioInterfered += t;
    }

    public Mote getMote() {
      return mote;
    }

    public void dispose() {
      radio.deleteObserver(this);
      radio = null;
      mote = null;
    }

    public String toString() {
      return toString(true, true, false);
    }
    public String toString(boolean radioHW, boolean radioRXTX, boolean html) {
      long duration = simulation.getSimulationTime() - startTime;

      StringBuilder sb = new StringBuilder();
      String id = mote.getID() + " ";
      String procent = " %";
      if (html) {
        procent = "% ";
        sb.append("<b>" + mote.toString() + "</b><br>");
        id = "";
      }

      if (radioHW) {
        sb.append(id + "ON " + 
            (html?"":(radioOn + " us ")) +
            format.format(100.0*((double)radioOn/duration)) + procent);
        if (!html) {
          sb.append("\n");
        }
      }
      if (radioRXTX) {
        sb.append(id + "TX " + 
            (html?"":(radioTx + " us ")) +
            format.format(100.0*((double)radioTx/duration)) + procent);
        if (!html) {
          sb.append("\n");
        }
        sb.append(id + "RX " + 
            (html?"":(radioRx + " us ")) +
            format.format(100.0*((double)radioRx/duration)) + procent);
        if (!html) {
          sb.append("\n");
        }
        sb.append(id + "INT " + 
            (html?"":(radioInterfered + " us ")) +
            format.format(100.0*((double)radioInterfered/duration)) + procent);
        if (!html) {
          sb.append("\n");
        }
      }
      if (html) {
        sb.append("<br>");
      }
      return sb.toString();
    }
  }

  private MoteTracker createMoteTracker(Mote mote) {
    final Radio moteRadio = mote.getInterfaces().getRadio();
    if (moteRadio == null) {
      return null;
    }

    /* Initial state */
    final long initialTime = simulation.getSimulationTime();
    final boolean initialRadioOn = moteRadio.isReceiverOn();
    final RadioState initialRadioState;
    if (moteRadio.isTransmitting()) {
      initialRadioState = RadioState.TRANSMITTING;
    } else if (moteRadio.isReceiving()) {
      initialRadioState = RadioState.RECEIVING;
    } else if (moteRadio.isInterfered()){
      initialRadioState = RadioState.INTERFERED;
    } else {
      initialRadioState = RadioState.IDLE;
    }

    /* Radio observer */
    MoteTracker tracker = new MoteTracker(mote, moteRadio, initialTime) {
      private boolean radioOn = initialRadioOn;
      private long radioOnTime = initialTime;
      private RadioState radioState = initialRadioState;
      private long radioStateTime = initialTime;

      public void update(Observable o, Object arg) {
        long now = simulation.getSimulationTime();
        RadioEvent nowRadioEvent = moteRadio.getLastEvent();

        /* Radio HW events */
        if (nowRadioEvent == RadioEvent.HW_ON) {
          if (radioOn) {
            /*logger.warn(getMote() + ": Radio was already on");*/
            accumulateRadioOn(now - radioOnTime);
          }
          radioOn = true;
          radioOnTime = now;
          return;
        }
        if (nowRadioEvent == RadioEvent.HW_OFF) {
          if (!radioOn) {
            /*logger.warn(getMote() + ": Radio was already off");*/
            return;
          }
          accumulateRadioOn(now - radioOnTime);
          radioOn = false;
          radioOnTime = now;
          return;
        }

        /* Radio state */
        if (radioState == RadioState.TRANSMITTING) {
          accumulateRadioTx(now - radioStateTime);
        } else if (radioState == RadioState.RECEIVING) {
          accumulateRadioRx(now - radioStateTime);
        } else if (radioState == RadioState.INTERFERED) {
          accumulateRadioIntefered(now - radioStateTime);
        }
        radioStateTime = now;
        if (radio.isTransmitting()) {
          radioState = RadioState.TRANSMITTING;
        } else if (!radio.isReceiverOn()) {
          radioState = RadioState.IDLE;
        } else if (radio.isInterfered()) {
          radioState = RadioState.INTERFERED;
        } else if (radio.isReceiving()) {
          radioState = RadioState.RECEIVING;
        } else {
          radioState = RadioState.IDLE;
        }          
      }
    };
    return tracker;
  }

  public void reset() {
  	while (moteTrackers.size() > 0) {
  		removeMote(moteTrackers.get(0).mote);
  	}
  	for (Mote m: simulation.getMotes()) {
  		addMote(m);
  	}
  }
  private void addMote(Mote mote) {
    if (mote == null) {
      return;
    }

    MoteTracker t = createMoteTracker(mote);
    if (t != null) {
      moteTrackers.add(t);
    }

    setTitle("PowerTracker: " + moteTrackers.size() + " motes");
  }

  private void removeMote(Mote mote) {
    /* Remove mote tracker(s) */
    MoteTracker[] trackers = moteTrackers.toArray(new MoteTracker[0]);
    for (MoteTracker t: trackers) {
      if (t.getMote() == mote) {
        t.dispose();
        moteTrackers.remove(t);
      }
    }

    setTitle("PowerTracker: " + moteTrackers.size() + " motes");
  }

  public void closePlugin() {
    /* Remove repaint timer */
    repaintTimer.stop();

    simulation.getEventCentral().removeMoteCountListener(moteCountListener);

    /* Remove mote trackers */
    for (Mote m: simulation.getMotes()) {
      removeMote(m);
    }
    if (!moteTrackers.isEmpty()) {
      logger.fatal("Mote observers not cleaned up correctly");
      for (MoteTracker t: moteTrackers.toArray(new MoteTracker[0])) {
        t.dispose();
      }
    }
  }

  public enum RadioState {
    IDLE, RECEIVING, TRANSMITTING, INTERFERED
  }

  private Timer repaintTimer = new Timer(POWERTRACKER_UPDATE_INTERVAL, new ActionListener() {
    public void actionPerformed(ActionEvent e) {
      String stats = radioStatistics(true, true, true, false);
      if (stats == null) {
        label.setText("No data");
        maxLabel.setText("");
        return;
      }
      String[] arr = stats.split(";");
      label.setText(arr[0]);
      if (arr.length > 1) {
        maxLabel.setText(arr[1]);
      } else {
        maxLabel.setText("");
      }
    }
  });

  public Collection<Element> getConfigXML() {
    return null;
  }
  public boolean setConfigXML(Collection<Element> configXML, boolean visAvailable) {
    return true;
  }

}
