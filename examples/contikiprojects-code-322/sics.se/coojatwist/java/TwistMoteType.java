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

import java.awt.Container;
import java.io.IOException;

import net.tinyos.message.Message;
import net.tinyos.message.MoteIF;
import net.tinyos.message.Sender;
import net.tinyos.packet.BuildSource;
import net.tinyos.packet.PacketListenerIF;
import net.tinyos.packet.PhoenixError;
import net.tinyos.packet.PhoenixSource;
import net.tinyos.util.Messenger;

import org.apache.log4j.Logger;

import se.sics.cooja.AbstractionLevelDescription;
import se.sics.cooja.ClassDescription;
import se.sics.cooja.GUI;
import se.sics.cooja.Mote;
import se.sics.cooja.MoteInterface;
import se.sics.cooja.MoteType;
import se.sics.cooja.RadioPacket;
import se.sics.cooja.Simulation;
import se.sics.cooja.dialogs.MessageList;
import se.sics.cooja.dialogs.SerialUI;
import se.sics.cooja.interfaces.ApplicationSerialPort;
import se.sics.cooja.motes.AbstractApplicationMote;
import se.sics.cooja.motes.AbstractApplicationMoteType;
import ch.ethz.ssh2.Connection;
import ch.ethz.ssh2.LocalPortForwarder;

@ClassDescription("Twist Mote Type")
@AbstractionLevelDescription("Application level")
public class TwistMoteType extends AbstractApplicationMoteType {
  private static Logger logger = Logger.getLogger(TwistMoteType.class);
  private static boolean localConnection = false; /* No need for SSH tunnels */

  public TwistMoteType() {
    super();
  }

  public TwistMoteType(String identifier) {
    super(identifier);
    setDescription("Twist Mote Type #" + identifier);
  }

  public boolean configureAndInit(Container parentContainer,
      Simulation simulation, boolean visAvailable) 
  throws MoteTypeCreationException {
    if (!super.configureAndInit(parentContainer, simulation, visAvailable)) {
      return false;
    }
    setDescription("Twist Mote Type #" + getIdentifier());
    return true;
  }

  public Mote generateMote(Simulation simulation) {
    return new TwistMote(this, simulation);
  }

  public Class<? extends MoteInterface>[] getMoteInterfaceClasses() {
    /* Replace default log interface */
    Class<? extends MoteInterface>[] classes = super.getMoteInterfaceClasses();
    for (int i=0; i < classes.length; i++) {
      if (classes[i].getName().equals(ApplicationSerialPort.class.getName())) {
        classes[i] = TwistSerialMoteInterface.class;
        break;
      }
    }
    return classes;
  }

  public static class TwistMote extends AbstractApplicationMote {
    public TwistMote() {
      super();
    }
    public TwistMote(MoteType moteType, Simulation simulation) {
      super(moteType, simulation);
    }
    public void execute(long time) {
      /* Ignore */
    }
    public void receivedPacket(RadioPacket p) {
      /* Ignore */
    }
    public void sentPacket(RadioPacket p) {
      /* Ignore */
    }
    public String toString() {
      return "Twist " + getID();
    }
  }

  @ClassDescription("Twist TinyOS Serial Forwarder connection")
  public static class TwistSerialMoteInterface extends SerialUI {
    public static final boolean REMOVE_TINYOS_FORMATTING = true;

    private Mote mote;

    /* Port forwarder */
    private Connection conn = null;
    private LocalPortForwarder localPortForwarder = null;

    /* Serial Forwarder */
    private PhoenixSource phoenixSource = null;

    public TwistSerialMoteInterface(Mote mote) {
      this.mote = mote;
      disconnect();
    }

    /**
     * Disconnect port forwarder and serial forwarder
     */
    public void disconnect() {
      if (localPortForwarder != null) {
        try {
          localPortForwarder.close();
        } catch (IOException e) {
          logger.warn("Error while closing port forwarder: " + e.getMessage(), e);
        }
        localPortForwarder = null;
      }
      if (conn != null) {
        conn.close();
        conn = null;
      }

      if (phoenixSource != null) {
        phoenixSource.shutdown();
        phoenixSource = null;
      }
    }

    /**
     * @param hostname Host name e.g. www.twist.tu-berlin.de
     * @param username User name e.g. twistextern
     * @param password Password
     */
    public void connect(String hostname, String username, String password, boolean resurrect)
    throws IOException {
      int port = 9000 + mote.getID();
      String serialForwarderAddress = "sf@localhost:" + port;

      localConnection = hostname.equals("localhost");
      
      if (!localConnection) {
        GUI.setProgressMessage("SSH tunnel: " + hostname + "#" + serialForwarderAddress, MessageList.NORMAL);

        /* Setup port forwarder via ssh */
        logger.info("Setting up port forwarder: \"" + port + ":" + hostname + ":" + port + "\"");
        try {
          /* Connect */
          conn = new Connection(hostname);
          conn.connect();
          if (!conn.authenticateWithPassword(username, password)) {
            throw new IOException("Authentication failed: " + username + "/******");
          }

          /* Create port forwarder */
          localPortForwarder = conn.createLocalPortForwarder(port, "localhost", port);
        } catch (IOException e) {
          throw (IOException) new IOException("Port forwarding failed: " + e.getMessage()).initCause(e);
        }
      }
      
      /* Connect to "local" (forwarded) serial port with TinyOS Serial Forwarder */
      logger.info("Connecting to \"local\" Serial Forwarder: " + serialForwarderAddress);
      phoenixSource = BuildSource.makePhoenix(serialForwarderAddress, new Messenger() {
        public void message(String s) {
          byte[] bytes = ("Phoenix message: " + s).getBytes();
          for (byte b: bytes) {
            dataReceived(b);
          }
          dataReceived('\n');
        }
      });
      phoenixSource.setPacketErrorHandler(new PhoenixError() {
        public void error(IOException e) {
//          byte[] bytes = ("Phoenix error: " + e.getMessage()).getBytes();
//          for (byte b: bytes) {
//            dataReceived(b);
//          }
//          dataReceived('\n');
//          logger.warn("Phoenix error: " + e.getMessage(), e);
//          disconnect();
        }
      });
      phoenixSource.registerPacketListener(new PacketListenerIF() {
        public void packetReceived(final byte[] packet) {
          mote.getSimulation().invokeSimulationThread(new Runnable() {
            public void run() {
              /*logger.debug("RX:\n" + StringUtils.hexDump(packet));*/

              int skip = 0;
              if (REMOVE_TINYOS_FORMATTING) {
                /* XXX Quick hack: may not work for all types of messages! */
                skip = 8;
              }

              for (byte b: packet) {
                if (skip-- > 0) continue;
                /*String s = "0" + Integer.toHexString(b&0xff);
                dataReceived(s.charAt(s.length()-2));
                dataReceived(s.charAt(s.length()-1));
                dataReceived(' ');*/
                
                dataReceived(b);
              }
            }
          });
        }
      });
      phoenixSource.start();
      if (resurrect) {
        phoenixSource.setResurrection();
      }
    }

    public boolean isConnected() {
      if (!localConnection) {
        if (conn == null) {
          return false;
        }
        if (!conn.isAuthenticationComplete()) {
          return false;
        }
      }

      if (phoenixSource == null) {
        return false;
      }
      return true;
    }

    public void removed() {
      super.removed();
      disconnect();
    }

    public void writeArray(byte[] s) {
      if (!isConnected()) {
        logger.warn("Not connected");
        return;
      }
      /* TODO XXX Untested code */
      try {
        Sender sender = new Sender(phoenixSource);
        Message message = new Message(s);
        sender.send(MoteIF.TOS_BCAST_ADDR, message);
        /*logger.debug("To:\n" + StringUtils.hexDump(message.dataGet()));*/
      } catch (IOException e) {
        logger.fatal("Error writing packet: " + e.getMessage(), e);
      }
    }

    private byte[] outBuf = new byte[5]; 
    private int outBufCounter = 0;
    public void writeByte(byte b) {
      if (!isConnected()) {
        logger.warn("Not connected");
        return;
      }

      /* TODO XXX Untested code */
      outBuf[outBufCounter++] = b;
      if (outBufCounter >= outBuf.length || (char)b == '\n') {
        byte[] tmp = new byte[outBufCounter];
        System.arraycopy(outBuf, 0, tmp, 0, outBufCounter);
        writeArray(tmp);
        
        outBufCounter = 0;
      }
    }
    public void writeString(String s) {
      if (!isConnected()) {
        logger.warn(getMote() + ": Not connected");
        return;
      }
      try {
        Sender sender = new Sender(phoenixSource);
        Message message = new Message((s + "\n").getBytes());
        sender.send(MoteIF.TOS_BCAST_ADDR, message);
        /*logger.debug("To:\n" + StringUtils.hexDump(message.dataGet()));*/
      } catch (IOException e) {
        logger.fatal("Error writing packet: " + e.getMessage(), e);
      }
    }

    public Mote getMote() {
      return mote;
    }
  }
}
