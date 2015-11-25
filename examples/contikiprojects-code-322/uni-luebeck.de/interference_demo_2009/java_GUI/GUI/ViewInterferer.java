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
 

package GUI;

import javax.swing.*;
import java.awt.*;
import java.io.*;
import java.net.*;

public class ViewInterferer extends JPanel implements Runnable{

  public InputStream inputstr;
  public int interfering=0, direction=-1, power=0, channel=0;
  public static int[] power_array = {-50,-48,-42,-25,-21,-21,-21,-18,-15,-15,-14,-13,-9,-9,-9,-7,-7,-6,-5,-5,-5,-5,-4,-3,-2,-2,-2,-1,-1,-1,-1,0};
  public static int MARGIN_BOTTOM = 25;
  public static int MARGIN_TOP = 20;
  public static int MARGIN_RIGHT = 60;
  public static int MARGIN_LEFT = 40;
  public static int POWER_MAX_VALUE = Math.abs(power_array[0])+Math.abs(power_array[power_array.length-1]);  
 
  public ViewInterferer() {
  }
  
  public void paint(Graphics g) {
	  	Graphics2D g2 = (Graphics2D) g;	  
	    int h = getHeight();
	    int w = getWidth();   
	    System.out.println("Height: " + h + " - Width: " + w);
	    double factor = (h - ((MARGIN_TOP+MARGIN_BOTTOM)*1.0)) / POWER_MAX_VALUE;

	    // Set white background in the plot
	    g.setColor(Color.white);     
	    g.fillRect(0, 0, w, h); 
	    
	    if(interfering == 1){
		    // Set the interferer power rectangle
		    g.setColor(Color.green);   
		    int width_rect = 25;
		    int height_rect = (int) ((Math.abs(power_array[0])+power_array[power])*factor);
	    	g.fillRect(w-width_rect-MARGIN_RIGHT,h-MARGIN_BOTTOM-height_rect,50,height_rect+1);
	    	
	        // Gradient example (ytics background)
	        GradientPaint greytowhite = new GradientPaint(w-MARGIN_RIGHT,0,Color.WHITE,w, 0,Color.lightGray, false);
	        g2.setPaint(greytowhite);
	        g2.fillRect(w-MARGIN_RIGHT, 0, w, h);
	    	
	        // Write the y-axis with dBm
	        int distance_from_rectangle = 10;        
	        int ytics = 5; // How many tics on the y-axis
	        int tics_value = ((Math.abs(power_array[0])+Math.abs(power_array[power_array.length-1])))/ytics;
	        for(int i=0;i<=ytics;i++){    	    	
	        	g.setColor(Color.red);
	        	g.drawString(String.valueOf((power_array[power_array.length-1]-i*tics_value)+"dBm"), (int) (w-MARGIN_RIGHT+distance_from_rectangle), (int) (h+((power_array[0]+i*tics_value)*factor))-MARGIN_BOTTOM);
	        	// Dashed line for the ytics
	        	final float dash1[] = {2.0f};
	            final BasicStroke dashed = new BasicStroke(1.0f, BasicStroke.CAP_BUTT, BasicStroke.JOIN_MITER, 10.0f, dash1, 0.0f);        
	        	g2.setStroke(dashed);
	        	g2.drawLine(w-width_rect-MARGIN_RIGHT,(int) (h+((power_array[0]+i*tics_value)*factor))-MARGIN_BOTTOM,(int) (w-MARGIN_RIGHT), (int) (h+((power_array[0]+i*tics_value)*factor))-MARGIN_BOTTOM);        
	        }
	        
	        // Write the x-axis with power value
	        g.setColor(Color.magenta);	 
		    Font font = new Font("Arial", Font.PLAIN, 11);    
		    g.setFont(font);
	        int start_x_axis = 4;
	        g.drawString(String.valueOf("TX Power [" + power + "]"), w-width_rect-MARGIN_RIGHT, (int) h-start_x_axis);
	    }
	    
	    
        // Rectangle direction
	    if((direction == 0)||(direction == 1)){
		    Font font = new Font("Arial", Font.PLAIN, 11);    
		    g.setFont(font);
	        g.setColor(Color.blue);
	        int direct_bottom = h-MARGIN_BOTTOM-120;
	        int direct_width = 14;
	        int direct_height = 50;
	        int triangle_height = 18;
	        int triangle_halfwidth = 14;
	        g2.setStroke(new BasicStroke(2));
	        g.fillRect(MARGIN_LEFT+triangle_halfwidth,direct_bottom,direct_width,direct_height);
		    // Write the x-axis for the direction	 
	        g.drawString(String.valueOf("Click"), 12,direct_bottom+(direct_height/2)-3);
	        g.drawString(String.valueOf("direction"), 4,direct_bottom+(direct_height/2)+7);
	    
	    
		    // Arrow up/down
	        if(direction == 0){ // Arrow up
			    Polygon p = new Polygon();
			    p.addPoint(MARGIN_LEFT,direct_bottom);
			    p.addPoint(MARGIN_LEFT+triangle_halfwidth+direct_width+triangle_halfwidth,direct_bottom);
			    p.addPoint(MARGIN_LEFT+triangle_halfwidth+(direct_width/2),direct_bottom-triangle_height);
			    g.fillPolygon(p);
			    g.drawPolygon(p);
	        }
	        else { // Arrow down
			    Polygon p2 = new Polygon();
			    p2.addPoint(MARGIN_LEFT,direct_bottom+direct_height);
			    p2.addPoint(MARGIN_LEFT+triangle_halfwidth+direct_width+triangle_halfwidth,direct_bottom+direct_height);
			    p2.addPoint(MARGIN_LEFT+triangle_halfwidth+(direct_width/2),direct_bottom+triangle_height+direct_height);
			    g.fillPolygon(p2);
			    g.drawPolygon(p2);
	        }
	    }
        else{ // Direction now known yet    	
        }
          		    
	    // Status of the interferer
	    Font font = new Font("Arial", Font.PLAIN, 14);    
	    g.setFont(font);
        g.setColor(Color.GRAY);
	    if(interfering == 0){
	    	g.drawString(String.valueOf("Status: OFF"), 4,h-20);
	    }
	    else {
	    	g.drawString(String.valueOf("Status: ON"), 4,h-20);
	    	if(channel!=0)
	    		g.drawString(String.valueOf("Channel: " + channel), 4,h-50);
	    }
  }
  
  public void handleInput() {	
    BufferedReader reader = new BufferedReader(new InputStreamReader(inputstr));
    while(true) {
        String line;        
		try {
			line = reader.readLine();				
			if(line!=null){
				 System.out.println(line);
				 if(line.startsWith("Transmission power set to ")){
					 String part = line.substring(26);
		  			 power = Integer.parseInt(part); 
		  			 interfering = 1;
		  			 System.out.println("POWER");
				 }
				 if(line.startsWith("New direction = ")){
					 String part = line.substring(16);
		  			 direction = Integer.parseInt(part); 
		  			 System.out.println("DIRECTION");
				 }
				 if(line.startsWith("Starting interfering with the ")){
		  			 interfering = 1;
		  			 System.out.println("INTERFERING");
				 }	  	
				 if(line.startsWith("Ready... channel ")){
					 String part = line.substring(17);
		  			 channel = Integer.parseInt(part); 
		  			 interfering = 0;
		  			 power = 0;
		  			 direction = 0;
		  			 System.out.println("RESTARTED" + channel);
				 }					 				
				 repaint();
			}
		} catch (IOException e) {
			//e.printStackTrace(); /* Report error, but do not fail... */
		}
    }
  }

  public void run(){
		this.handleInput();	
  }
 
}
