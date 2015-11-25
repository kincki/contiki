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
import gnu.io.*; // for rxtxSerial library
 
public class GetNoise {
	
	// Constant variables
	public static final String COM_PORT_1 = "COM11"; // Interferer
	public static final String COM_PORT_2 = "COM15"; // Noise scanner
	
	public static final int PRINTING_TIME = 35000;	// Milliseconds (how often I should print)
	
	// Array containing the noise values for different transmission power values
	public static final int ARRAY_SIZE = 34;
	protected static double sum_noise[] = new double[ARRAY_SIZE];
	protected static double count_noise[] = new double[ARRAY_SIZE];
	protected static double avg_noise[] = new double[ARRAY_SIZE];
	
	// Variable updated by the interferer
	protected static int current_power = -1;
	
	// Serial communication
	static Enumeration<CommPortIdentifier> portList;
	public SerialPort PortInterferer;
	public SerialPort PortNoise;


	// Recognizing hardware port
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
    
    
    // Detecting available Serial ports
    public int detect_ports(String port_to_check){
    	int port_found = 0;
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
            System.out.println(portId.getName()  +  " - " +  getPortTypeName(portId.getPortType()) +  " - " + availability);
            countPorts++;
            if(port_to_check.compareTo(portId.getName()) == 0){
            	port_found = 1;
            }
        }
       	System.out.println("Total number of ports detected: " + countPorts);
        return port_found;
    }
    

	public void ContactPorts(String port1, String port2) {
		CommPortIdentifier portIdentifier, portIdentifier2;		
		try {
			// Contact interferer (port 1)
			System.out.println("Connecting to " + port1);
			portIdentifier = CommPortIdentifier.getPortIdentifier(port1);
			CommPort commPort = portIdentifier.open(this.getClass().getName(),2000);          
			PortInterferer = (SerialPort) commPort;			
			PortInterferer.setSerialPortParams(115200,SerialPort.DATABITS_8,SerialPort.STOPBITS_1,SerialPort.PARITY_NONE);
			// Thread for the first serial port (Interferer)
			(new Thread() {
				public void run() {
					try	{
						BufferedReader reader = new BufferedReader(new InputStreamReader(PortInterferer.getInputStream()));
						while(true) {
					        String line;        
								try {		
									line = reader.readLine();
									current_power = Integer.valueOf(line);							
								}
								catch (IOException e) { // Do not fail... 
								}
						}
					}
					catch (IOException e) { // Do not fail... 
					}
				}			
			}).start();
			
			
			// Contact noise scanner (port 2)
			System.out.println("Connecting to " + port2);
			portIdentifier2 = CommPortIdentifier.getPortIdentifier(port2);
			CommPort commPort2 = portIdentifier2.open(this.getClass().getName(),2000);          
			PortNoise = (SerialPort) commPort2;
			PortNoise.setSerialPortParams(115200,SerialPort.DATABITS_8,SerialPort.STOPBITS_1,SerialPort.PARITY_NONE);				
			// Thread for the second serial port
			(new Thread() {
				public void run() {
					try {
						BufferedReader reader = new BufferedReader(new InputStreamReader(PortNoise.getInputStream()));
						while(true) {
					        String line;        		
							try {		
								line = reader.readLine();
								if(current_power != -1){
									sum_noise[current_power] += Integer.valueOf(line);
									count_noise[current_power]++;
									avg_noise[current_power] = sum_noise[current_power] / count_noise[current_power];												
								}
							}
							catch (IOException e) { // Do not fail... 
							}
						}
					}
					catch (IOException e) { // Do not fail...  
					}
				}			
			}).start();
			
			
			// Thread to print the noise values
			System.out.println("==========================================");
			(new Thread() {
				public void run() {					
					while(true) {
						try {
							Thread.sleep(PRINTING_TIME);
							System.out.println("OFF: " + avg_noise[ARRAY_SIZE-2]);
							int i=0;
							for(i=0;i<ARRAY_SIZE-2;i++){
								System.out.println(i + ": " + avg_noise[i]);
							}
							// Resetting array
							for(i=0;i<ARRAY_SIZE-1;i++){
								avg_noise[i] = 0;
								sum_noise[i] = 0;
								count_noise[i] = 0;
							}
						} catch (InterruptedException e) {							
						}
					}
				}
			}).start();
					
		} catch (NoSuchPortException e2) { System.out.println("The selected port does not exist!");
		} catch (PortInUseException e3) { System.out.println("The selected port is already in use!");			
		} catch (UnsupportedCommOperationException e4) { System.out.println("Unsupported CommOperation!");
		} catch (Exception e5) { System.out.println("Generic exception!"); e5.printStackTrace();
		} 
	} 


	public GetNoise() {
	    // Verify availability of serial ports
		System.out.println("Looking for ports " + COM_PORT_1 + " and " + COM_PORT_2);
	    int serial = detect_ports(COM_PORT_1);
	    serial += detect_ports(COM_PORT_2);
	    if(serial == 2){
	    	System.out.println("Ports " + COM_PORT_1 + " and " + COM_PORT_2 + " found.");
	    	System.out.println("Contacting ports...");
	    	ContactPorts(COM_PORT_1, COM_PORT_2);
	    }
	    else{
	    	System.out.println("Error: cannot find ports!");	
	    }	           
	}

	
	public static void main(String[] args) {		
		GetNoise main_program = new GetNoise();		
		int i=0;
		for(i=0;i<ARRAY_SIZE-1;i++){
			avg_noise[i] = 0;
			sum_noise[i] = 0;
			count_noise[i] = 0;
		}
	} 

}