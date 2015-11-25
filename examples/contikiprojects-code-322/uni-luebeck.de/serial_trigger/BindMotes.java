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
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ''AS IS'' AND
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
 *
 */

import java.io.*;
import java.util.*;
import javax.swing.*;
import java.awt.*;
import java.awt.event.*;
import gnu.io.*; // for rxtxSerial library

public class BindMotes extends JPanel implements ActionListener{			
	static Enumeration<CommPortIdentifier> portList;
	public static JFrame frame; 
	public static JComboBox combo1,combo2;
	public static JButton button1,button2,button3;
	public static JInternalFrame iframe;
	public static String comportmain = "", comportanswering = "";
	 
	
    static String getPortTypeName (int portType)
    {
        switch (portType)
        {
            case CommPortIdentifier.PORT_I2C: return "I2C port";
            case CommPortIdentifier.PORT_PARALLEL: return "Parallel port";
            case CommPortIdentifier.PORT_RAW: return "Raw port";
            case CommPortIdentifier.PORT_RS485: return "RS485 port";
            case CommPortIdentifier.PORT_SERIAL: return "Serial port";
            default: return "Unknown Type port";
        }
    }
    
    protected JMenuBar createMenuBar() {
        JMenuBar menuBar = new JMenuBar();
        JMenu menu = new JMenu("File");
        menu.setMnemonic(KeyEvent.VK_F);
        menuBar.add(menu);
        //Set up the first menu item.
        JMenuItem menuItem = new JMenuItem("Quit");
        menuItem.setMnemonic(KeyEvent.VK_Q);
        menuItem.setAccelerator(KeyStroke.getKeyStroke(KeyEvent.VK_Q, ActionEvent.ALT_MASK));
        menuItem.setActionCommand("quit");
        menuItem.addActionListener(this);
        menu.add(menuItem); 
        return menuBar;
    }
    
    public String[] detect_ports(){
    	String existingPorts[] = new String[16];
    	int countPorts = 0;
		portList = CommPortIdentifier.getPortIdentifiers(); // Parse ports		
        while (portList.hasMoreElements()) 
        {
            CommPortIdentifier portId = portList.nextElement();
            String availability = "";
            try {
                CommPort thePort = portId.open("CommUtil", 50);
                thePort.close();
                availability = "Available";             
            } catch (PortInUseException e) {
                availability = "Already in use";
            } catch (Exception e) {
                availability = "Cannot open port";
            }
            existingPorts[countPorts] = portId.getName()  +  " - " +  getPortTypeName(portId.getPortType()) +  " - " + availability;            
            countPorts++;
        }
        // Load the Ports string
        String Ports[] = new String[countPorts];
	    for (int i=0;i<countPorts;i++){
	    	Ports[i] = existingPorts[i];
	    }
	    return Ports;
    }
    
	public static void main(String[] args) {		
		BindMotes interface_GUI = new BindMotes();		
	}  
	
	public void actionPerformed(ActionEvent e) {
		// Serial port for main mote
	    if ("setmain".equals(e.getActionCommand())) {
	        combo1.setEnabled(false);
	        button1.setEnabled(false); 
	        comportmain = (String) combo1.getSelectedItem();
			comportmain = comportmain.substring(0, comportmain.indexOf('-')-1);
			System.out.printf("Main mote: %s\n",comportmain);			
	    }
	    // Serial port for answering mote
	    if ("setanswering".equals(e.getActionCommand())) {
	        combo2.setEnabled(false);
	        button2.setEnabled(false);
			comportanswering = (String) combo2.getSelectedItem();
			comportanswering = comportanswering.substring(0, comportanswering.indexOf('-')-1);
			System.out.printf("Answering mote: %s\n",comportanswering);		        
	    }	   
	    // Start the binding
	    if ("setstart".equals(e.getActionCommand())) {
	    	button3.setEnabled(false);
	    	System.out.printf("Binding motes: %s and %s\n",comportmain,comportanswering);
	        CommPortIdentifier portmainIdentifier, portansweringIndentifier;
			try {
				portmainIdentifier = CommPortIdentifier.getPortIdentifier(comportmain);
			    CommPort commMainPort = portmainIdentifier.open(this.getClass().getName(),2000);          
			    SerialPort serialPortmain = (SerialPort) commMainPort;
			    serialPortmain.setSerialPortParams(115200,SerialPort.DATABITS_8,SerialPort.STOPBITS_1,SerialPort.PARITY_NONE);
			     	   
			    portansweringIndentifier = CommPortIdentifier.getPortIdentifier(comportanswering);
			    CommPort commAnsweringPort = portansweringIndentifier.open(this.getClass().getName(),2000);          
			    SerialPort serialPortanswering = (SerialPort) commAnsweringPort;
			    serialPortanswering.setSerialPortParams(115200,SerialPort.DATABITS_8,SerialPort.STOPBITS_1,SerialPort.PARITY_NONE);
			    serialPortanswering.notifyOnOutputEmpty(true);
			   
			    BufferedReader reader = new BufferedReader(new InputStreamReader(serialPortmain.getInputStream()));
			    BufferedReader reader2 = new BufferedReader(new InputStreamReader(serialPortanswering.getInputStream()));
			    while(true) {
			        String line, line2;        
					try {			
						// Read from the serial port of the main mote
						line = reader.readLine();
						
						if(line.contains("#")){				
							line = line + System.getProperty("line.separator");
							serialPortanswering.getOutputStream().write(line.getBytes());
						}		
						else{
							System.out.println(line);
						}
						// Read from the serial port of the answering mote
						// line2 = reader2.readLine();
						// System.out.println(line2);
					} catch (IOException e1) { 
						// e1.printStackTrace();
					} catch (Exception e2) { 
						// e2.printStackTrace();
					}
			    }
			    
			} catch (NoSuchPortException e2) { JOptionPane.showMessageDialog(frame,"The selected port does not exist!");
			} catch (gnu.io.PortInUseException e3) { JOptionPane.showMessageDialog(frame,"The selected port is already in use!");
			} catch (IOException e4) { JOptionPane.showMessageDialog(frame,"IO Exception!");				
			} catch (UnsupportedCommOperationException e5) { JOptionPane.showMessageDialog(frame,"Unsupported CommOperation!");
			}   
	    }	    
	    // Quit
	    if ("quit".equals(e.getActionCommand())) { 
            System.exit(0);
        }
	} 


	public BindMotes() {
		//mainmote = new ReadPort();
	    
		frame = new JFrame("Serial mote communication");
        frame.setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE);  
        frame.setBounds(300, 300, 605, 250);
        frame.setVisible(true);
	    frame.setJMenuBar(createMenuBar()); // Add the menu to the Frame  
	    
	    // Creating an JDesktopPane that contains an internal Frame that contains a panel
	    JDesktopPane desk;	   
	    JPanel panel = new JPanel();	    	   
	    iframe = new JInternalFrame("Configuration", true,false,false,false); // Caption,caption,minimize,maximize,close	    
	    panel = new JPanel();	
	     	
	    String[] Allports = detect_ports();
	    
	    combo1 = new JComboBox(Allports);
	    combo1.setBackground(Color.gray);
	    combo1.setForeground(Color.black);
	    combo1.setBounds(10, 10, 300, 20);
	    combo2 = new JComboBox(Allports);
	    combo2.setBackground(Color.gray);
	    combo2.setForeground(Color.black);
	    combo2.setBounds(10, 40, 300, 20);
	    button1 = new JButton("Set Main Mote"); 
	    button1.setActionCommand("setmain");
	    button1.addActionListener(this);	    		    	    	     
	    button1.setBounds(320, 10, 250, 20);
	    button2 = new JButton("Set Answering Mote");
	    button2.setActionCommand("setanswering");
	    button2.addActionListener(this);
	    button2.setBounds(320, 40, 250, 20);
	    button3 = new JButton("Bind the two Motes!");
	    button3.setActionCommand("setstart");
	    button3.addActionListener(this);	    	    
	    button3.setBounds(170, 85, 250, 40);
	    	 	   
	    panel.add(combo1);
	    panel.add(combo2);
	    panel.add(button1);
	    panel.add(button2);
	    panel.add(button3);	        
	    panel.setLayout(null);	    
	    iframe.add(panel);
	    iframe.setBounds(0,0,590,190);
	    iframe.setVisible(true);	 	    	    	   	    	    	   	 		  	    
	    desk = new JDesktopPane();
	    desk.add(iframe);
	    frame.add(desk);	         
	}

}