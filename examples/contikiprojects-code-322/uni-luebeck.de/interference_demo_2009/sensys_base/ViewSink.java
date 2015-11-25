/*
 * Copyright (c) 2010, University of Luebeck, Germany.
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
 * Author: Carlo Alberto Boano <cboano@iti.uni-luebeck.de>
 *
 */

import javax.swing.*;
import java.awt.*;
import java.io.*;

public class ViewSink extends JPanel{

  public InputStream inputstr;
  public int seqno=0, noise_sr=0, rssi_sr=0, noise_rb=0, rssi_rb=0, snr_rb=0, lqi_rb=0;
  public int prr_sr=0, prr_sr1=0, prr_sr2=0, prr_sr3=0;
  public int snr_sr=0, snr_sr1=0, snr_sr2=0, snr_sr3=0;
  public int lqi_sr=0, lqi_sr1=0, lqi_sr2=0, lqi_sr3=0;
  public static int MARGIN_BOTTOM = 25;
  public static int MARGIN_TOP = 25;
  public static int MARGIN_RIGHT = 20;
  public static int MARGIN_LEFT = 20;
  public static int MARGIN_LEFT_RECTANGLES = 40;
  public static int MAX_SNR = 75;
  public static int MAX_LQI = 110;
  public static int MIN_LQI = 50;
  public static int MAX_PRR = 100;

  public ViewSink() {
  }
  
  public void paint(Graphics g) {
	  	Graphics2D g2 = (Graphics2D) g;	  
	    int h = getHeight();
	    int w = getWidth();    

	    // Set white background in the plot
	    g.setColor(Color.white);     
	    g.fillRect(0, 0, w, h); 
	    
	    // Set color and font   	  
	    if(snr_sr == -1){
	    	g.setColor(Color.red);
	    	snr_sr = 0;
	    	lqi_sr = 0;
	    }
	    else{
	    	g.setColor(Color.green);
	   }	        
	    
	    // Write TX-RX	   
	    int height_rectangles = 20;
	    int snr_height = 35;
	    int lqi_height = 65;
	    int prr_height = 95;
	    int maxwidth_rectangle = 150;
	    // Labels
	    Font font = new Font("Arial", Font.PLAIN, 18);    
	    g2.setFont(font);
	    // Sequence number
	    g.drawString(String.valueOf("Communication TX - RX:  [" + seqno + "]"), MARGIN_LEFT,MARGIN_TOP);	    
	    font = new Font("Arial", Font.PLAIN, 14);    
	    g2.setFont(font);
	    g.setColor(Color.black); 
	    g.drawString(String.valueOf("SNR:"), MARGIN_LEFT,MARGIN_TOP+snr_height);	    	    
	    g.drawString(String.valueOf("LQI:"), MARGIN_LEFT,MARGIN_TOP+lqi_height);
	    g.drawString(String.valueOf("PRR:"), MARGIN_LEFT,MARGIN_TOP+prr_height);
	    // Full rectangles
        // Gradient example (ytics background)
        GradientPaint greytowhite = new GradientPaint(MARGIN_LEFT+MARGIN_LEFT_RECTANGLES,0,Color.WHITE,maxwidth_rectangle, 0,Color.lightGray, false);
        g2.setPaint(greytowhite);        
	    g.fillRect(MARGIN_LEFT+MARGIN_LEFT_RECTANGLES, MARGIN_TOP+snr_height-height_rectangles, maxwidth_rectangle, height_rectangles);
	    g.fillRect(MARGIN_LEFT+MARGIN_LEFT_RECTANGLES, MARGIN_TOP+lqi_height-height_rectangles, maxwidth_rectangle, height_rectangles); 	    
	    g.fillRect(MARGIN_LEFT+MARGIN_LEFT_RECTANGLES, MARGIN_TOP+prr_height-height_rectangles, maxwidth_rectangle, height_rectangles); 
	    // Filled in rectangles
	    g.setColor(Color.gray);
	    double current_snr = snr_sr * (maxwidth_rectangle*1.0/MAX_SNR);	    
	    double current_lqi = (lqi_sr-MIN_LQI) * (maxwidth_rectangle*1.0/(MAX_LQI-MIN_LQI));
	    if(lqi_sr==0) current_lqi = 0; // trick to avoid that the LQI bar is below 0 (the rectangle starts from MIN_LQI)
	    double current_prr = prr_sr * (maxwidth_rectangle*1.0/MAX_PRR);
	    g.fillRect(MARGIN_LEFT+MARGIN_LEFT_RECTANGLES, MARGIN_TOP+snr_height-height_rectangles, (int) current_snr+1, height_rectangles);
	    g.drawString(String.valueOf(snr_sr + " dBm"), 220,55);
	    g.fillRect(MARGIN_LEFT+MARGIN_LEFT_RECTANGLES, MARGIN_TOP+lqi_height-height_rectangles, (int) current_lqi+1, height_rectangles);
	    g.drawString(String.valueOf(lqi_sr + " CCI"), 220,85);
	    g.fillRect(MARGIN_LEFT+MARGIN_LEFT_RECTANGLES, MARGIN_TOP+prr_height-height_rectangles, (int) current_prr+1, height_rectangles);
	    g.drawString(String.valueOf(prr_sr + "%"), 220,115);

	    
	    
	    // WRITE RX-BASE
	    // Labels
	    font = new Font("Arial", Font.PLAIN, 18);    
	    g2.setFont(font);
	    g.drawString(String.valueOf("Communication RX - BASE:"), MARGIN_LEFT,MARGIN_TOP+prr_height+43);
	    greytowhite = new GradientPaint(MARGIN_LEFT+MARGIN_LEFT_RECTANGLES,0,Color.WHITE,maxwidth_rectangle, 0,Color.lightGray, false);
        g2.setPaint(greytowhite);        
        maxwidth_rectangle = 120;
	    g.fillRect(250,MARGIN_TOP+prr_height+45-height_rectangles,maxwidth_rectangle,height_rectangles);
	    double current_snr_rb = snr_rb * (maxwidth_rectangle*1.0/MAX_SNR);
	    g.setColor(Color.gray);
	    g.fillRect(250, MARGIN_TOP+prr_height+45-height_rectangles, (int) (current_snr_rb)+1, height_rectangles);
	    font = new Font("Times", Font.BOLD, 14);    
	    g2.setFont(font);
	    g.setColor(Color.black);
	    g.drawString(String.valueOf(snr_rb + " dBm"), 250+(maxwidth_rectangle/3),MARGIN_TOP+prr_height+45-6);
  }
  
  public void handleInput() {	
    BufferedReader reader = new BufferedReader(new InputStreamReader(System.in));
    while(true) {
        String line;        
		try {
			line = reader.readLine();	
			if((line!=null)&&(line.startsWith("BASE:"))){
	  			 System.out.println(line);
	  			 String[] parts = line.substring(5).split(",");
	  			 seqno = Integer.parseInt(parts[0]);
	  			 noise_sr = Integer.parseInt(parts[1]);
	  			 rssi_sr = Integer.parseInt(parts[2]);
	  			 lqi_sr2 = lqi_sr1;
	  			 lqi_sr3 = lqi_sr2;
	  			 lqi_sr1 = Integer.parseInt(parts[3]);
	  			 lqi_sr = (lqi_sr1+lqi_sr2+lqi_sr3)/3;
	  			 snr_sr2 = snr_sr1;
	  			 snr_sr3 = snr_sr2;
	  			 snr_sr1 = Integer.parseInt(parts[4]);
	  			 snr_sr = (snr_sr1+snr_sr2+snr_sr3)/3;
	  			 prr_sr2 = prr_sr1;
	  			 prr_sr3 = prr_sr2;
	  			 prr_sr1 = Integer.parseInt(parts[5]);	  	
	  			 prr_sr = (prr_sr1+prr_sr2+prr_sr3)/3;
	  			 rssi_rb = Integer.parseInt(parts[6]);
	  			 noise_rb = Integer.parseInt(parts[7]);
	  			 lqi_rb = Integer.parseInt(parts[8]);
	  			 snr_rb = Integer.parseInt(parts[9]); 
				 repaint();
			}
		} catch (IOException e) {
			snr_sr = 0;
			lqi_sr = 0;
			//prr_sr = 0;
			snr_rb = 0;
			//e.printStackTrace(); /* Report error, but do not fail... */
		}
    }
  }


  public static void main(String[] args) throws IOException {
    JFrame win = new JFrame("Sink");
    ViewSink panel;
    win.setBounds(0,195,390,205);
    win.getContentPane().add(panel = new ViewSink());
    win.setVisible(true);
    win.setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE);
    panel.handleInput();
  }
 
}
