/*
 * Copyright (c) 2010, Friedrich-Alexander University Erlangen, Germany
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
 * This file is part of the Contiki operating system.
 *
 */

/**
 * @{
 * \file
 *         Slider interface for the light-sensors of a sky-mote in COOJA.
 * \author
 *         Moritz Strübe <Moritz.Struebe@informatik.uni-erlangen.de>
 */

package se.sics.cooja.mspmote.interfaces;


import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import java.awt.GridLayout;
import java.util.Collection;

import javax.swing.JSlider;
import javax.swing.JLabel;
import javax.swing.JPanel;


import org.apache.log4j.Logger;
import org.jdom.Element;

import se.sics.cooja.ClassDescription;
import se.sics.cooja.Mote;
import se.sics.cooja.MoteInterface;
import se.sics.cooja.Simulation;
import se.sics.cooja.interfaces.Button;
import se.sics.cooja.mspmote.MspMoteTimeEvent;
import se.sics.cooja.mspmote.SkyMote;
import se.sics.mspsim.core.ADC12;
import se.sics.mspsim.core.ADCInput;
import se.sics.mspsim.core.IOUnit;

@ClassDescription("Light sensor")
public class SkyLight extends MoteInterface {
  private static Logger logger = Logger.getLogger(SkyLight.class);

  private SkyMote skyMote;
  
  protected class ADCtest implements ADCInput {
	  private int input;
	  public ADCtest(int inp) {
		  input = inp;
	  }
	  
	  public int nextData(){
		  return input;
	  }
	  
  }
  
  protected class ADCret implements ADCInput {
	  private JSlider myslider; 
	  
	  ADCret(JSlider slider){
		  myslider = slider;
	  }
	  
	  public int nextData(){
		  //
		  if(myslider == null){
			  return 1023;
		  } else {
			  //logger.debug("Getting data: " + myslider.getValue() );
			  return myslider.getValue();
		  }
	  }
	  
	  
  }
  
  public SkyLight(Mote mote) {
	 skyMote = (SkyMote) mote;
  }

  public JPanel getInterfaceVisualizer() {
    JPanel panel = new JPanel(new GridLayout(0,2));
    final JSlider sADC1 = new JSlider(JSlider.HORIZONTAL, 0, 1023, 200);
    final JSlider sADC2 = new JSlider(JSlider.HORIZONTAL, 0, 1023, 200);
    panel.add(new JLabel("Light1"));
    panel.add(sADC1);
    panel.add(new JLabel("Light2"));
    panel.add(sADC2);
    
    IOUnit adc = skyMote.getCPU().getIOUnit("ADC12");
    if (adc instanceof ADC12) {
     	 ((ADC12) adc).setADCInput(4, new ADCret(sADC1));
      	 ((ADC12) adc).setADCInput(5, new ADCret(sADC2)); 
    	
    /*	
   	 ((ADC12) adc).setADCInput(0, new ADCtest(0));
   	 ((ADC12) adc).setADCInput(1, new ADCtest(1));
   	 ((ADC12) adc).setADCInput(2, new ADCtest(2));
  	 ((ADC12) adc).setADCInput(3, new ADCtest(3));
  	 ((ADC12) adc).setADCInput(4, new ADCtest(4));
  	 ((ADC12) adc).setADCInput(5, new ADCtest(5));
  	 ((ADC12) adc).setADCInput(6, new ADCtest(6));
  	 ((ADC12) adc).setADCInput(7, new ADCtest(7));
  	 ((ADC12) adc).setADCInput(8, new ADCtest(8));
  	 ((ADC12) adc).setADCInput(9, new ADCtest(9));
  	 ((ADC12) adc).setADCInput(10, new ADCtest(10));
  	 ((ADC12) adc).setADCInput(11, new ADCtest(11));
  	 ((ADC12) adc).setADCInput(12, new ADCtest(12));
  	 ((ADC12) adc).setADCInput(13, new ADCtest(13));
  	 ((ADC12) adc).setADCInput(14, new ADCtest(14));
  	 ((ADC12) adc).setADCInput(15, new ADCtest(15));*/
    }
    
    
    return panel;
  }

  public void releaseInterfaceVisualizer(JPanel panel) {
  }

  public Collection<Element> getConfigXML() {
    return null;
  }

  public void setConfigXML(Collection<Element> configXML, boolean visAvailable) {
  }
  


}

