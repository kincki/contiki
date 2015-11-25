/*
 * Copyright (c) 2011, Swedish Institute of Computer Science.
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
 */

import se.sics.cooja.MoteTimeEvent;
import se.sics.cooja.MoteType;
import se.sics.cooja.RadioPacket;
import se.sics.cooja.Simulation;
import se.sics.cooja.interfaces.ApplicationRadio;
import se.sics.cooja.motes.AbstractApplicationMote;

/**
 * @author Fredrik Osterlind
 */
public class Noise extends AbstractApplicationMote {

	public static final long REFRESH_DURATION = 100*Simulation.MILLISECOND;
	public static final int NOISE_SIGNAL_STRENTH = -20;
	
  private Simulation simulation = null;
  private ApplicationRadio radio = null;

  private boolean active = false;
  
  public Noise() {
    super();
    getSimulation().invokeSimulationThread(init);
  }

  public Noise(MoteType moteType, Simulation sim) {
    super(moteType, sim);
    sim.invokeSimulationThread(init);
  }

  private Runnable init = new Runnable() {
		public void run() {
      simulation = getSimulation();
      radio = (ApplicationRadio) getInterfaces().getRadio();
		}
	};
  
  public void execute(long time) {
  }

  public void setActive(boolean active) {
  	this.active = active;
  	if (!active) {
  		radio.setNoiseLevel(Integer.MIN_VALUE);
  		return;
  	}

		radio.setNoiseLevel(NOISE_SIGNAL_STRENTH);

    /* Transmit packet periodically */
    simulation.scheduleEvent(
        refreshEvent,
        simulation.getSimulationTime()
    );

  }
  private MoteTimeEvent refreshEvent = new MoteTimeEvent(this, 0) {
    public void execute(long t) {
    	if (!active) {
    		radio.setNoiseLevel(Integer.MIN_VALUE);
    		return;
    	}

  		radio.setNoiseLevel(NOISE_SIGNAL_STRENTH);

      /* Transmit packet periodically */
      simulation.scheduleEvent(
          refreshEvent,
          t + REFRESH_DURATION
      );
    }
  };

  public void receivedPacket(RadioPacket p) {
  }

  public void sentPacket(RadioPacket p) {
  }

  public String toString() {
    return "Noise " + getID();
  }
}

