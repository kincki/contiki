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
import java.io.*;
import java.util.*;
import javax.swing.*;
import java.awt.*;
import java.awt.event.*;
import gnu.io.*; // for rxtxSerial library

public class GUI extends JPanel
implements ActionListener{			
	static Enumeration<CommPortIdentifier> portList;
	public static JFrame frame; 
	public static JComboBox combo1,combo2,combo3,combo4,combo5;
	public static JButton button1,button2,button3,button4,button5;
	public static JInternalFrame iframe,iframe2,iframe3,iframe4,iframe5,iframe6;
	public static ViewRSSI spectrum;
	public static ViewSink sink1, sink2;
	public static ViewInterferer interferer1, interferer2;
	 
	
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
        // Set up the first menu item.
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
		GUI interface_GUI = new GUI();		
	} 
	
	public void actionPerformed(ActionEvent e) {
		// We should compute the available COM ports each time we select one (but slow!)		
		// for(int i=0;i<combo1.getItemCount();i++){
	    //      combo1.removeItemAt(i);
		// }
		//combo1 = new JComboBox(detect_ports());
		
		// Serial port for player 1
	    if ("set1".equals(e.getActionCommand())) {
	    	// Disable button 1 and show panel
	        combo1.setEnabled(false);
	        button1.setEnabled(false);
	        iframe2.setVisible(true);
	        // Start communication for player 1
	        CommPortIdentifier portIdentifier;
			try {
				String porta = (String)combo1.getSelectedItem();
				porta = porta.substring(0, porta.indexOf('-')-1);
				System.out.printf("PORTA: %s\n",porta);
				portIdentifier = CommPortIdentifier.getPortIdentifier(porta);
			    CommPort commPort = portIdentifier.open(this.getClass().getName(),2000);          
			    SerialPort serialPort = (SerialPort) commPort;
			    serialPort.setSerialPortParams(115200,SerialPort.DATABITS_8,SerialPort.STOPBITS_1,SerialPort.PARITY_NONE);			    
			    sink1.inputstr = serialPort.getInputStream(); 
			    Thread t = new Thread(sink1);	
			    t.start();
			} catch (NoSuchPortException e2) { JOptionPane.showMessageDialog(frame,"The selected port does not exist!");
			} catch (PortInUseException e3) { JOptionPane.showMessageDialog(frame,"The selected port is already in use!");
			} catch (IOException e4) { JOptionPane.showMessageDialog(frame,"IO Exception!");				
			} catch (UnsupportedCommOperationException e5) { JOptionPane.showMessageDialog(frame,"Unsupported CommOperation!");
			} 		        
	    }
	    // Serial port for interferer 1
	    if ("set2".equals(e.getActionCommand())) {
	    	// Disable button 2 and show panel
	        combo2.setEnabled(false);
	        button2.setEnabled(false);
	        iframe3.setVisible(true);
	        // Start communication for interferer 1
	        CommPortIdentifier portIdentifier;
			try {
				String porta = (String)combo2.getSelectedItem();
				porta = porta.substring(0, porta.indexOf('-')-1);
				System.out.printf("PORTA: %s\n",porta);
				portIdentifier = CommPortIdentifier.getPortIdentifier(porta);
			    CommPort commPort = portIdentifier.open(this.getClass().getName(),2000);          
			    SerialPort serialPort = (SerialPort) commPort;
			    serialPort.setSerialPortParams(115200,SerialPort.DATABITS_8,SerialPort.STOPBITS_1,SerialPort.PARITY_NONE);			    
			    interferer1.inputstr = serialPort.getInputStream(); 
			    Thread t = new Thread(interferer1);	
			    t.start();
			} catch (NoSuchPortException e2) { JOptionPane.showMessageDialog(frame,"The selected port does not exist!");
			} catch (PortInUseException e3) { JOptionPane.showMessageDialog(frame,"The selected port is already in use!");
			} catch (IOException e4) { JOptionPane.showMessageDialog(frame,"IO Exception!");				
			} catch (UnsupportedCommOperationException e5) { JOptionPane.showMessageDialog(frame,"Unsupported CommOperation!");
			}		
	    }	   
	    // Serial port for player 2
	    if ("set3".equals(e.getActionCommand())) {
	    	// Disable button 3 and show panel
	        combo3.setEnabled(false);
	        button3.setEnabled(false);
	        iframe4.setVisible(true);
	        // Start communication for player 2
	        CommPortIdentifier portIdentifier;
			try {
				String porta = (String)combo3.getSelectedItem();
				porta = porta.substring(0, porta.indexOf('-')-1);
				System.out.printf("PORTA: %s\n",porta);
				portIdentifier = CommPortIdentifier.getPortIdentifier(porta);
			    CommPort commPort = portIdentifier.open(this.getClass().getName(),2000);          
			    SerialPort serialPort = (SerialPort) commPort;
			    serialPort.setSerialPortParams(115200,SerialPort.DATABITS_8,SerialPort.STOPBITS_1,SerialPort.PARITY_NONE);			    
			    sink2.inputstr = serialPort.getInputStream(); 
			    Thread t = new Thread(sink2);	
			    t.start();
			} catch (NoSuchPortException e2) { JOptionPane.showMessageDialog(frame,"The selected port does not exist!");
			} catch (PortInUseException e3) { JOptionPane.showMessageDialog(frame,"The selected port is already in use!");
			} catch (IOException e4) { JOptionPane.showMessageDialog(frame,"IO Exception!");				
			} catch (UnsupportedCommOperationException e5) { JOptionPane.showMessageDialog(frame,"Unsupported CommOperation!");
			}
	    }	
	    // Serial port for interferer 2
	    if ("set4".equals(e.getActionCommand())) {
	    	// Disable button 4 and show panel
	        combo4.setEnabled(false);
	        button4.setEnabled(false);
	        iframe5.setVisible(true);
	        // Start communication for interferer 2
	        CommPortIdentifier portIdentifier;
			try {
				String porta = (String)combo4.getSelectedItem();
				porta = porta.substring(0, porta.indexOf('-')-1);
				System.out.printf("PORTA: %s\n",porta);
				portIdentifier = CommPortIdentifier.getPortIdentifier(porta);
			    CommPort commPort = portIdentifier.open(this.getClass().getName(),2000);          
			    SerialPort serialPort = (SerialPort) commPort;
			    serialPort.setSerialPortParams(115200,SerialPort.DATABITS_8,SerialPort.STOPBITS_1,SerialPort.PARITY_NONE);			    
			    interferer2.inputstr = serialPort.getInputStream(); 
			    Thread t = new Thread(interferer2);	
			    t.start();
			} catch (NoSuchPortException e2) { JOptionPane.showMessageDialog(frame,"The selected port does not exist!");
			} catch (PortInUseException e3) { JOptionPane.showMessageDialog(frame,"The selected port is already in use!");
			} catch (IOException e4) { JOptionPane.showMessageDialog(frame,"IO Exception!");				
			} catch (UnsupportedCommOperationException e5) { JOptionPane.showMessageDialog(frame,"Unsupported CommOperation!");
			}		
	    }	
	    // Serial port for the spectrum analyzer
	    if ("set5".equals(e.getActionCommand())) {
	    	// Disable button 5 and show panel
	        combo5.setEnabled(false);
	        button5.setEnabled(false);
	        iframe6.setVisible(true);
	        // Start communication for spectrum analyzer
	        CommPortIdentifier portIdentifier;
			try {
				String porta = (String)combo5.getSelectedItem();
				porta = porta.substring(0, porta.indexOf('-')-1);
				System.out.printf("PORTA: %s\n",porta);
				portIdentifier = CommPortIdentifier.getPortIdentifier(porta);
			    CommPort commPort = portIdentifier.open(this.getClass().getName(),2000);          
			    SerialPort serialPort = (SerialPort) commPort;
			    serialPort.setSerialPortParams(115200,SerialPort.DATABITS_8,SerialPort.STOPBITS_1,SerialPort.PARITY_NONE);
			    spectrum.inputstr = serialPort.getInputStream(); 
			    Thread t = new Thread(spectrum);	        
			    t.start();	        
			} catch (NoSuchPortException e2) { JOptionPane.showMessageDialog(frame,"The selected port does not exist!");
			} catch (PortInUseException e3) { JOptionPane.showMessageDialog(frame,"The selected port is already in use!");
			} catch (IOException e4) { JOptionPane.showMessageDialog(frame,"IO Exception!");				
			} catch (UnsupportedCommOperationException e5) { JOptionPane.showMessageDialog(frame,"Unsupported CommOperation!");
			} 			
	    }	    
	    // Quit
	    if ("quit".equals(e.getActionCommand())) { 
            System.exit(0);
        }
	} 



	public GUI() {
		frame = new JFrame("SenSys 2009 - Interference demo");
        frame.setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE);  
        //Make the big window be indented insert pixels from each edge of the screen.
        int inset = 115;
        Dimension screenSize = Toolkit.getDefaultToolkit().getScreenSize();        
        frame.setBounds(inset, inset, screenSize.width  - inset*2, screenSize.height - inset*2);
        frame.setVisible(true);
        
        // Add the menu to the Frame  
	    frame.setJMenuBar(createMenuBar());
	    
	    // Creating an JDesktopPane that contains an internal Frame that contains a panel
	    JDesktopPane desk;	   
	    JPanel panel = new JPanel();	    	   
	    iframe = new JInternalFrame("Configuration", true,false,false,false); // Caption,caption,minimize,maximize,close	    
	    panel = new JPanel();	
	    
	    // Add the combo1 to the panel	    	
	    String[] Allports = detect_ports();
	    combo1 = new JComboBox(Allports);
	    combo1.setBackground(Color.gray);
	    combo1.setForeground(Color.black);	    
	    panel.add(combo1);	   
	    // Button for the combo1
	    button1 = new JButton("Set for Player 1"); 
	    button1.setActionCommand("set1");
	    button1.addActionListener(this);
	    panel.add(button1);
	    
	    // Add the combo2 to the panel	    	    	      
	    combo2 = new JComboBox(Allports);
	    combo2.setBackground(Color.gray);
	    combo2.setForeground(Color.black);	    
	    panel.add(combo2);	   
	    // Button for the combo2
	    button2 = new JButton("Set for Interferer 1");
	    button2.setActionCommand("set2");
	    button2.addActionListener(this);
	    panel.add(button2);
	    
	    // Add the combo3 to the panel	    	    	      
	    combo3 = new JComboBox(Allports);
	    combo3.setBackground(Color.gray);
	    combo3.setForeground(Color.black);	    	       
	    panel.add(combo3);	   
	    // Button for the combo3
	    button3 = new JButton("Set for Player 2");
	    button3.setActionCommand("set3");
	    button3.addActionListener(this);
	    panel.add(button3);
	    
	    // Add the combo4 to the panel	    	    	      
	    combo4 = new JComboBox(Allports);
	    combo4.setBackground(Color.gray);
	    combo4.setForeground(Color.black);	    
	    panel.add(combo4);	   
	    // Button for the combo4
	    button4 = new JButton("Set for Interferer 2");
	    button4.setActionCommand("set4");
	    button4.addActionListener(this);
	    panel.add(button4);
	    
	    // Add the combo5 to the panel	    	    	      
	    combo5 = new JComboBox(Allports);
	    combo5.setBackground(Color.gray);
	    combo5.setForeground(Color.black);	    
	    panel.add(combo5);	   
	    // Button for the combo5
	    button5 = new JButton("Set for Spectrum Analyzer");
	    button5.setActionCommand("set5");
	    button5.addActionListener(this);
	    panel.add(button5);
	    
	    panel.setLayout(null);
	    button1.setBounds(320, 10, 250, 20);
	    button2.setBounds(320, 40, 250, 20);
	    button3.setBounds(320, 70, 250, 20);
	    button4.setBounds(320, 100, 250, 20);
	    button5.setBounds(320, 130, 250, 20);
	    combo1.setBounds(10, 10, 300, 20);
	    combo2.setBounds(10, 40, 300, 20);
	    combo3.setBounds(10, 70, 300, 20);
	    combo4.setBounds(10, 100, 300, 20);
	    combo5.setBounds(10, 130, 300, 20);
	    iframe.add(panel);
	    iframe.setBounds(0,0,590,190);
	    iframe.setVisible(true);
	 	    	    	   	    	    	   
	    iframe2 = new JInternalFrame("Player 1", true,false,false,false); // Caption,caption,minimize,maximize,close	    	    
	    sink1 = new ViewSink();
	    iframe2.add(sink1);
	    iframe2.setBounds(0,195,390,205);
	    iframe2.setVisible(false);
	    
	    iframe3 = new JInternalFrame("Interferer 1", true,false,false,false); // Caption,caption,minimize,maximize,close	    	    
	    interferer1 = new ViewInterferer();
	    iframe3.add(interferer1);
	    iframe3.setBounds(400,195,190,205);
	    iframe3.setVisible(false);
	    	    	   	    	   
	    iframe4 = new JInternalFrame("Player 2", true,false,false,false); // Caption,caption,minimize,maximize,close	    
	    sink2 = new ViewSink();
	    iframe4.add(sink2);
	    iframe4.setBounds(0,405,390,200);
	    iframe4.setVisible(false);
	    	    	   
	    iframe5 = new JInternalFrame("Interferer 2", true,false,false,false); // Caption,caption,minimize,maximize,close	    
	    interferer2 = new ViewInterferer();
	    iframe5.add(interferer2);
	    iframe5.setBounds(400,405,190,200);
	    iframe5.setVisible(false);
	    	    
	    iframe6 = new JInternalFrame("Spectrum Analyzer", true,false,false,false); // Caption,caption,minimize,maximize,close
	    spectrum = new ViewRSSI();
	    iframe6.add(spectrum);
	    iframe6.setBounds(595,0,595,605);
	    iframe6.setVisible(false);	 		    
	    
	    desk = new JDesktopPane();
	    desk.add(iframe);
	    desk.add(iframe2);
	    desk.add(iframe3);
	    desk.add(iframe4);
	    desk.add(iframe5);
	    desk.add(iframe6);
	    frame.add(desk);	         
	}

}