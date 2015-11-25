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

import java.util.Arrays;

import se.sics.cooja.MoteType;
import se.sics.cooja.RadioPacket;
import se.sics.cooja.Simulation;
import se.sics.cooja.interfaces.ApplicationRadio;
import se.sics.cooja.motes.AbstractApplicationMote;

/**
 * @author Fredrik Osterlind
 */
public class Receiver extends AbstractApplicationMote {

  private Simulation simulation = null;
  private ApplicationRadio radio = null;

  public Receiver() {
    super();
    
    simulation.invokeSimulationThread(new Runnable() {
			public void run() {
	      simulation = getSimulation();
	      radio = (ApplicationRadio) getInterfaces().getRadio();
			}
		});
  }

  public Receiver(MoteType moteType, Simulation sim) {
    super(moteType, sim);
    
    sim.invokeSimulationThread(new Runnable() {
			public void run() {
	      simulation = getSimulation();
	      radio = (ApplicationRadio) getInterfaces().getRadio();
			}
		});
  }

  public void execute(long time) {
  }

  public void receivedPacket(RadioPacket p) {
    byte[] packetData = p.getPacketData();
    if (packetData == null) {
    	log("NO DATA");
    	return;
    }
    if (packetData.length != Transmitter.PACKET_DATA.getPacketData().length) {
    	log("BAD SIZE");
    	return;
    }

    boolean corrupted = 
    	!Arrays.equals(packetData,Transmitter.PACKET_DATA.getPacketData());
    if (corrupted) {
    	log("BAD CONTENTS");
    	return;
    }
    
    log("RX");
  }

  public void sentPacket(RadioPacket p) {
  }

  public String toString() {
    return "Receiver " + getID();
  }
}

