/*
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

import java.awt.BorderLayout;
import java.awt.Component;
import java.awt.Window;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import java.awt.event.MouseEvent;
import java.awt.event.MouseListener;
import java.io.BufferedReader;
import java.io.File;
import java.io.IOException;
import java.io.InputStreamReader;
import java.util.ArrayList;
import java.util.Collection;
import java.util.List;
//import java.util.Observer;


import javax.swing.AbstractAction;
import javax.swing.Action;
import javax.swing.BorderFactory;
import javax.swing.Box;
import javax.swing.DefaultListCellRenderer;
import javax.swing.JButton;
import javax.swing.JCheckBox;
import javax.swing.JComboBox;
import javax.swing.JDialog;
import javax.swing.JFileChooser;
import javax.swing.JLabel;
import javax.swing.JOptionPane;
import javax.swing.JPanel;
import javax.swing.JPasswordField;
import javax.swing.JProgressBar;
import javax.swing.JScrollPane;
import javax.swing.JTextArea;
import javax.swing.JTextField;
import javax.swing.SwingUtilities;
import javax.swing.Timer;
import javax.swing.filechooser.FileFilter;
import javax.swing.JTable;
import javax.swing.table.DefaultTableCellRenderer;
import javax.swing.table.DefaultTableModel;
import javax.swing.table.TableCellEditor;
import javax.swing.table.TableCellRenderer;
import javax.swing.table.TableModel;
import javax.swing.DefaultCellEditor;
import javax.swing.table.AbstractTableModel;
import javax.swing.event.ListSelectionEvent;
import javax.swing.event.ListSelectionListener;
import javax.swing.event.TableModelEvent;
import javax.swing.event.TableModelListener;

import org.apache.log4j.Logger;
import org.jdom.Element;

import se.sics.cooja.ClassDescription;
import se.sics.cooja.GUI;
import se.sics.cooja.Mote;
import se.sics.cooja.MoteType;
import se.sics.cooja.PluginType;
import se.sics.cooja.SimEventCentral.LogOutputEvent;
import se.sics.cooja.SimEventCentral.LogOutputListener;
import se.sics.cooja.SimEventCentral.MoteCountListener;
import se.sics.cooja.Simulation;
import se.sics.cooja.VisPlugin;
import se.sics.cooja.dialogs.CompileContiki;
import se.sics.cooja.dialogs.MessageList;
import se.sics.cooja.dialogs.MessageList.MessageContainer;

@ClassDescription("TWIST Testbed")
@PluginType(PluginType.SIM_PLUGIN)
public class TwistPlugin extends VisPlugin {
  private static final long serialVersionUID = 1L;
  protected static final String TITLE = "TWIST Testbed";
  private static Logger logger = Logger.getLogger(TwistPlugin.class);

  /* Twist website (2011-08-16):
   * Due to reconstruction work in the TKN building the following nodes are not 
   * currently accesible: 59, 60, 274, 275, 62, 64, 276, 277, 171, 174, 278 and 
   * 279. We apologize for the inconvenience! */
  static final String ALL_TWIST_MOTES = 
//    "10 11 12 13 15 79 80 81 82 83 84 85 86 87 88 89 90 91 92 93 94 95 96 97 99 100 101 102 103 137 138 139 140 141 142 143 144 145 146 147 148 149 150 151 152 153 154 185 186 187 188 189 190 191 192 193 194 195 196 197 198 199 200 202 203 204 205 206 207 208 209 211 212 213 214 215 216 218 220 221 222 223 224 225 228 229 230 231 240 241 249 250 251 252 262 272";
      "10 11 12 13 15 79 80 81 82 83 84 85 86 87 88 89 90 91 92 93 94 95 96 97 99 100 101 102 103 137 138 139 140 141 142 143 144 145 146 147 148 149 150 151 152 153 154 185 186 189 190 191 192 194 195 196 197 200 203 204 205 206 207 208 209 211 212 213 214 215 216 218 220 221 222 223 224 225 228 229 230 231 240 241 249 250 251 252 262 272";
  private static String curlAguments = "--cookie cookies.txt --cookie-jar cookies.txt";

  private Simulation simulation;
  private JTextField serverField, portsField, usernameField, webUsernameField;
  private JPasswordField passwordField, webPasswordField;
  private JCheckBox savePasswordsBox, autoconnectBox, resurrectBox;

  private JComboBox cbxBinTable = new JComboBox();
  private JCheckBox ckxEnabled = new JCheckBox();
  private List<String> binairiesList = new ArrayList<String>();
  private JTable tblSelectBinary;
  private String[] columnNames = {"ID", "Use?", "Binary File"};
  private final static int COLUMN_MOTEID = 0;
  private final static int COLUMN_ENABLED = 1;
  private final static int COLUMN_BINPATH = 2;
  private final static int COLUMN_COUNT = 3;
  private Boolean checkAll = true;
  private MoteCountListener moteCountListener;
    
  private LogOutputListener newMotesListener = new LogOutputListener() {
    public void moteWasAdded(Mote mote) {
      updateTitle();
    }
    public void moteWasRemoved(Mote mote) {
      updateTitle();
    }
    public void removedLogOutput(LogOutputEvent ev) {
    }
    public void newLogOutput(LogOutputEvent ev) {
      updateTitle();
    }
  };

  public TwistPlugin(Simulation simulation, GUI gui) {
    super(TITLE, gui);
    this.simulation = simulation;
    simulation.getEventCentral().addLogOutputListener(newMotesListener);
    updateTitle();

    /* Get curl version to create user-agent string */
    try {
      Process proc = Runtime.getRuntime().exec("curl -V");
      proc.waitFor();
      BufferedReader buf = new BufferedReader(new InputStreamReader(proc.getInputStream()));
      curlAguments += " \"-A" + buf.readLine() + " coojatwist\"";
    } catch(Exception e) {
      curlAguments += " \"-Acoojatwist\"";
    }
    
    /* GUI components */

    /* Server/ports/user/password */
    serverField = new JTextField();
    serverField.setToolTipText("Default: https://www.twist.tu-berlin.de:8000");
    serverField.setText("https://www.twist.tu-berlin.de:8000");

    portsField = new JTextField();
    portsField.setText("[getID() + 9000]");
    portsField.setEnabled(false);

    usernameField = new JTextField();
    usernameField.setToolTipText("Default: twistextern");
    usernameField.setText("twistextern");

    webUsernameField = new JTextField();

    passwordField = new JPasswordField();

    webPasswordField = new JPasswordField();

    savePasswordsBox = new JCheckBox("Save passwords (cleartext)", false);
    autoconnectBox = new JCheckBox("Autoconnect at simulation load", false);
    resurrectBox = new JCheckBox("Resurrect Serial Forwarder", false);
    
    /* Layout */
    Box settingsBox = Box.createVerticalBox();
    settingsBox.add(Box.createVerticalStrut(10));
    settingsBox.add(new JLabel("Server address:"));
    settingsBox.add(serverField);
    settingsBox.add(new JLabel("Server ports:"));
    settingsBox.add(portsField);
    settingsBox.add(new JLabel("SSH username:"));
    settingsBox.add(usernameField);
    settingsBox.add(new JLabel("SSH password:"));
    settingsBox.add(passwordField);
    settingsBox.add(new JLabel("Web username:"));
    settingsBox.add(webUsernameField);
    settingsBox.add(new JLabel("Web password:"));
    settingsBox.add(webPasswordField);
    settingsBox.add(savePasswordsBox);
    settingsBox.add(autoconnectBox);
    settingsBox.add(resurrectBox);
    settingsBox.add(Box.createVerticalStrut(20));
    settingsBox.add(Box.createVerticalGlue());

    Box controlBox = Box.createHorizontalBox();
    JComboBox coojaActionComboBox = new JComboBox();
    JComboBox configActionComboBox = new JComboBox();
    JComboBox twistActionComboBox = new JComboBox();
    
    coojaActionComboBox.addItem("Cooja actions:");
    coojaActionComboBox.addItem("--------");
    coojaActionComboBox.addItem(generateMotes);
    coojaActionComboBox.addItem(connectAction);
    coojaActionComboBox.addItem(disconnectAction);
    coojaActionComboBox.addActionListener(new ActionListener() {
      public void actionPerformed(ActionEvent e) {
        JComboBox actionComboBox = (JComboBox)e.getSource();
        if (actionComboBox.getSelectedIndex() < 2) {
          return;
        }
        Action a = (Action)actionComboBox.getSelectedItem();
        a.actionPerformed(null);
        actionComboBox.setSelectedIndex(0);
      }
    });
    
    configActionComboBox.addItem("Config actions:");
    configActionComboBox.addItem("--------");
    configActionComboBox.addItem(addBinAction);
    configActionComboBox.addItem(clearBinListAction);
    configActionComboBox.setSelectedIndex(0);
    configActionComboBox.addActionListener(new ActionListener() {
      public void actionPerformed(ActionEvent e) {
        JComboBox actionComboBox = (JComboBox)e.getSource();
        if (actionComboBox.getSelectedIndex() < 2) {
          return;
        }
        Action a = (Action)actionComboBox.getSelectedItem();
        a.actionPerformed(null);
        actionComboBox.setSelectedIndex(0);
      }
    });

    twistActionComboBox.addItem("Twist actions:");
    twistActionComboBox.addItem("--------");
    twistActionComboBox.addItem(uploadAction);
    twistActionComboBox.addItem(uploadMultiAction);
    twistActionComboBox.addItem(eraseAction);
    twistActionComboBox.addItem(resetAction);
    twistActionComboBox.addItem(powerOnAction);
    twistActionComboBox.addItem(powerOffAction);
    twistActionComboBox.addItem(traceStart);
    twistActionComboBox.addItem(traceStop);
    twistActionComboBox.setSelectedIndex(0);
    twistActionComboBox.addActionListener(new ActionListener() {
      public void actionPerformed(ActionEvent e) {
        JComboBox actionComboBox = (JComboBox)e.getSource();
        if (actionComboBox.getSelectedIndex() < 2) {
          return;
        }
        Action a = (Action)actionComboBox.getSelectedItem();
        a.actionPerformed(null);
        actionComboBox.setSelectedIndex(0);
      }
    });
    
    DefaultTableModel mdlBinTable = new DefaultTableModel();
    for(String name: columnNames){
    	mdlBinTable.addColumn(name);
    }
    
    tblSelectBinary = new JTable(mdlBinTable) {
      private static final long serialVersionUID = -4680013510092815210L;
      public TableCellEditor getCellEditor(int row, int column) {
        if(column == COLUMN_BINPATH) {
          cbxBinTable.setEditable(false);
          cbxBinTable.putClientProperty("JComboBox.isTableCellEditor", Boolean.TRUE);
        }
        return super.getCellEditor(row, column);
      }
      
      @Override
      public Class getColumnClass(int c) {
        switch (c) {
          case COLUMN_ENABLED:
            return Boolean.class;
          default:
            return String.class;
        }
      }
      {
        getTableHeader().addMouseListener(new MouseListener() {
          @Override
          public void mouseClicked(MouseEvent mouseEvent) {
            int index = convertColumnIndexToModel(columnAtPoint(mouseEvent.getPoint()));
            if (index >= 0) {
              //The user clicked on column #index
              if (index == COLUMN_ENABLED) {
                checkAll = !checkAll;
                for(int row=0; row<tblSelectBinary.getRowCount(); row++) {
                  tblSelectBinary.setValueAt(checkAll, row, COLUMN_ENABLED);
                }
              }
            }
          };
          @Override
          public void mouseExited(MouseEvent e) {}
          @Override
          public void mouseReleased(MouseEvent e) {}
          @Override
          public void mousePressed(MouseEvent e) {}
          @Override
          public void mouseEntered(MouseEvent e) {}
        });
      }
    };

    cbxBinTable.setEditable(true);
    BinTableCellRenderer renderer = new BinTableCellRenderer();
    tblSelectBinary.getColumnModel().getColumn(COLUMN_BINPATH).setCellEditor(new DefaultCellEditor(cbxBinTable));
    tblSelectBinary.getColumnModel().getColumn(COLUMN_BINPATH).setCellRenderer(renderer);
    tblSelectBinary.setAutoResizeMode(tblSelectBinary.AUTO_RESIZE_LAST_COLUMN);
    tblSelectBinary.getColumnModel().getColumn(COLUMN_MOTEID).setMinWidth(40);
    tblSelectBinary.getColumnModel().getColumn(COLUMN_MOTEID).setMaxWidth(80);
    tblSelectBinary.getColumnModel().getColumn(COLUMN_MOTEID).setPreferredWidth(40);
    tblSelectBinary.getColumnModel().getColumn(COLUMN_ENABLED).setCellEditor(new DefaultCellEditor(ckxEnabled));
    tblSelectBinary.getColumnModel().getColumn(COLUMN_ENABLED).setMinWidth(40);
    tblSelectBinary.getColumnModel().getColumn(COLUMN_ENABLED).setMaxWidth(80);
    tblSelectBinary.getColumnModel().getColumn(COLUMN_ENABLED).setPreferredWidth(40);

    JScrollPane binarySelectPane = new JScrollPane(tblSelectBinary);

    controlBox.add(coojaActionComboBox);
    controlBox.add(configActionComboBox);
    controlBox.add(twistActionComboBox);
    getContentPane().setLayout(new BorderLayout());
    add(BorderLayout.WEST, settingsBox);
    add(BorderLayout.CENTER, binarySelectPane);
    add(BorderLayout.SOUTH, controlBox);
    
    simulation.getEventCentral().addMoteCountListener(moteCountListener = new MoteCountListener() {
      public void moteWasAdded(Mote mote) {
        /* Add a row in the table model when a new TwistMote is created in the simulation */
        if (mote instanceof TwistMoteType.TwistMote) {
          Object[] rowData = {Integer.toString(mote.getID()), Boolean.TRUE, ""};
          ((DefaultTableModel)tblSelectBinary.getModel()).addRow(rowData);
        }
      }
      public void moteWasRemoved(Mote mote) {
        /* User deletes a mote from the simulation --> we remove the corresponding row from the table's model*/
        for (int row = 0; row < tblSelectBinary.getRowCount(); row++) {	
          if (tblSelectBinary.getValueAt(row, COLUMN_MOTEID).equals(Integer.toString(mote.getID()))) {
            ((DefaultTableModel)tblSelectBinary.getModel()).removeRow(row);
          }
        }
      }
    });
    
    pack();
  }
  
  private void updateTitle() {
    SwingUtilities.invokeLater(new Runnable() {
      int tot = 0;
      int connected = 0;
      public void run() {
        for (Mote m: getTwistMotes()) {
          TwistMoteType.TwistSerialMoteInterface s = m.getInterfaces().getInterfaceOfType(TwistMoteType.TwistSerialMoteInterface.class);
          tot++;
          if (s.isConnected()) {
            connected++;
          }
        }
        setTitle(TITLE + " (" + connected + "/" + tot + " connected)");
      }
    });
  }

  private Action generateMotes = new AbstractAction("Generate motes") {
    private static final long serialVersionUID = 1L;

    public void actionPerformed(ActionEvent e) {
      /* Add TWIST mote type */
      TwistMoteType moteType = null;
      for (MoteType mt: simulation.getMoteTypes()) {
        if (mt instanceof TwistMoteType) {
          moteType = (TwistMoteType) mt;
          break;
        }
      }
      if (moteType == null) {
        moteType = new TwistMoteType("autogenerated");
        simulation.addMoteType(moteType);
      }

      /* Add motes */
      String[] allMotes = ALL_TWIST_MOTES.split(" ");
      for (String idString: allMotes) {
        int id = Integer.parseInt(idString);
        Mote m = simulation.getMoteWithID(id);
        if (m != null) {
          if (!(m instanceof TwistMoteType.TwistMote)) {
            logger.warn("Mote " + m.getID() + " already exists but is not a TWIST mote: " + m);
          }
          continue;
        }
        
        m = moteType.generateMote(simulation);
        m.getInterfaces().getMoteID().setMoteID(id);
        m.getInterfaces().getPosition().setCoordinates(
            Math.random()*100.0,
            Math.random()*100.0,
            0
        );
        simulation.addMote(m);
      }
    }
    
    public String toString() {
      return (String) this.getValue(Action.NAME);
    };
  };
  
  private Action connectAction = new AbstractAction("Connect motes") {
    private static final long serialVersionUID = 1L;
    public void actionPerformed(ActionEvent e) {
      connectAll();
    }
    public String toString() {
      return (String) this.getValue(Action.NAME);
    };
  };
  
  private Action disconnectAction = new AbstractAction("Disconnect motes") {
    private static final long serialVersionUID = 1L;
    public void actionPerformed(ActionEvent e) {
      disconnectAll();
    }
    public String toString() {
      return (String) this.getValue(Action.NAME);
    };
  };

  private Action addBinAction = new AbstractAction("Add Binary") {
    private static final long serialVersionUID = 1L;
    public void actionPerformed(ActionEvent e) {
      String last = GUI.getExternalToolsSetting("TWIST_BINARY");
      File lastFile = null;
      if (last != null) {
        lastFile = simulation.getGUI().restorePortablePath(new File(last));
      }
      JFileChooser fileChooser = new JFileChooser();
      if (lastFile != null && lastFile.exists()) {
        fileChooser.setSelectedFile(lastFile);
      }
      fileChooser.addChoosableFileFilter(new FileFilter() {
        public String getDescription() {
          return "Sky binary";
        }
        public boolean accept(File f) {
          if (f.isDirectory()) {
            return true;
          }
          if (f.getName().endsWith(".sky")) {
            return true;
          }
          if (f.getName().endsWith(".ihex")) {
            return true;
          }
          return false;
        }
      });
      int returnVal = fileChooser.showOpenDialog(TwistPlugin.this);
      if (returnVal != JFileChooser.APPROVE_OPTION) {
        return;
      }
      final File file = fileChooser.getSelectedFile();
      GUI.setExternalToolsSetting("TWIST_BINARY", simulation.getGUI().createPortablePath(file).getPath());
      addBinary(file.getPath());
    }
    public String toString() {
      return (String) this.getValue(Action.NAME);
    };
  };

  private Action clearBinListAction = new AbstractAction("Clear Binairies") {
    private static final long serialVersionUID = 1L;
    public void actionPerformed(ActionEvent e) {
      tblSelectBinary.clearSelection();
      binairiesList.clear();
      cbxBinTable.removeAllItems();
      for (int i = 0; i < tblSelectBinary.getRowCount(); i++) {	
        tblSelectBinary.setValueAt("", i, COLUMN_BINPATH);
      }
    }
    public String toString() {
      return (String) this.getValue(Action.NAME);
    };
  };
  
  private Action uploadAction = new AbstractAction("Upload single binary to all motes") {
    private static final long serialVersionUID = 1L;
    public void actionPerformed(ActionEvent e) {

      String last = GUI.getExternalToolsSetting("TWIST_BINARY");
      File lastFile = null;
      if (last != null) {
        lastFile = simulation.getGUI().restorePortablePath(new File(last));
      }
      JFileChooser fileChooser = new JFileChooser();
      if (lastFile != null && lastFile.exists()) {
        fileChooser.setSelectedFile(lastFile);
      }
      fileChooser.addChoosableFileFilter(new FileFilter() {
        public String getDescription() {
          return "Sky binary";
        }
        public boolean accept(File f) {
          if (f.isDirectory()) {
            return true;
          }
          if (f.getName().endsWith(".sky")) {
            return true;
          }
          if (f.getName().endsWith(".ihex")) {
            return true;
          }
          return false;
        }
      });
      int returnVal = fileChooser.showOpenDialog(TwistPlugin.this);
      if (returnVal != JFileChooser.APPROVE_OPTION) {
        return;
      }
      final File file = fileChooser.getSelectedFile();
      GUI.setExternalToolsSetting("TWIST_BINARY", simulation.getGUI().createPortablePath(file).getPath());

      twistCommand(
          webUsernameField.getText(),
          new String(webPasswordField.getPassword()),
          serverField.getText(),
          "-F ctrl.grp1.image=@" + file.getName() + " -F ctrl.grp1.sfversion=2 -F ctrl.grp1.sfspeed=115200 -F install=Install",
          file.getParentFile(),
          "Install done",
          "install"
      );
    }
        
    public String toString() {
      return (String) this.getValue(Action.NAME);
    };
  };

  private Action uploadMultiAction = new AbstractAction("Upload binairies as set in table") {
    private static final long serialVersionUID = 1L;
    public void actionPerformed(ActionEvent e) {

      String strWebAction = "";
      int groupCount = 0;
    
      for (int i=0; i<binairiesList.size(); i++)
      {
        String groupMotes = getMoteIdsFromBinary(binairiesList.get(i));
        if (groupMotes=="")
          continue;
        groupCount++;
        String groupName = "grp" + Integer.toString(groupCount);
        strWebAction += "-F ctrl." + groupName + ".image=@" + new File(binairiesList.get(i)).getName() + " -F ctrl." + groupName + ".sfversion=2 -F ctrl." + groupName + ".sfspeed=115200";
        strWebAction += " -F \"ctrl." + groupName + ".nodes=" + groupMotes + "\" ";
      }
      
      if (groupCount > 0) {
        strWebAction += "-F install=Install";
        System.out.println(strWebAction);
        System.out.flush();
	      
        twistCommand(
          webUsernameField.getText(),
          new String(webPasswordField.getPassword()),
          serverField.getText(),
          strWebAction,
          new File(binairiesList.get(0)).getParentFile(),
          "Install done",
          "install"
        );
      }
      else{
        //XXX NO BINARY SELECTED
      }
    }
        
    public String toString() {
      return (String) this.getValue(Action.NAME);
    };
  };

  
  private Action eraseAction = new AbstractAction("Erase nodes") {
    private static final long serialVersionUID = 1L;
    public void actionPerformed(ActionEvent e) {
      twistCommand(
          webUsernameField.getText(),
          new String(webPasswordField.getPassword()),
          serverField.getText(),
          "-F ctrl.grp1.sfversion=2 -F ctrl.grp1.sfspeed=115200 -F \"erase=Erase\"",
          null,
          "Erase done",
          "erase"
      );
    }
    public String toString() {
      return (String) this.getValue(Action.NAME);
    };
  };
  
  private Action resetAction = new AbstractAction("Reset nodes") {
    private static final long serialVersionUID = 1L;
    public void actionPerformed(ActionEvent e) {
      twistCommand(
          webUsernameField.getText(),
          new String(webPasswordField.getPassword()),
          serverField.getText(),
          "-F ctrl.grp1.sfversion=2 -F ctrl.grp1.sfspeed=115200 -F \"reset=Reset\"",
          null,
          "Reset done",
          "reset"
      );
    }
    public String toString() {
      return (String) this.getValue(Action.NAME);
    };
  };
  
  private Action powerOnAction = new AbstractAction("Power nodes on") {
    private static final long serialVersionUID = 1L;
    public void actionPerformed(ActionEvent e) {
      twistCommand(
          webUsernameField.getText(),
          new String(webPasswordField.getPassword()),
          serverField.getText(),
          "-F ctrl.grp1.sfversion=2 -F ctrl.grp1.sfspeed=115200 -F \"power_on=Power On\"",
          null,
          "Power up done",
          "power up"
      );
    }
    public String toString() {
      return (String) this.getValue(Action.NAME);
    };
  };
  
  private Action powerOffAction = new AbstractAction("Power nodes off") {
    private static final long serialVersionUID = 1L;
    public void actionPerformed(ActionEvent e) {
      twistCommand(
          webUsernameField.getText(),
          new String(webPasswordField.getPassword()),
          serverField.getText(),
          "-F ctrl.grp1.sfversion=2 -F ctrl.grp1.sfspeed=115200 -F \"power_off=Power Off\"",
          null,
          "Power down done",
          "power down"
      );
    }
    public String toString() {
      return (String) this.getValue(Action.NAME);
    };
  };
    
  private Action traceStart = new AbstractAction("Start tracing") {
    private static final long serialVersionUID = 1L;
    public void actionPerformed(ActionEvent e) {
      twistCommand(
          webUsernameField.getText(),
          new String(webPasswordField.getPassword()),
          serverField.getText(),
          "-F ctrl.grp1.sfversion=2 -F ctrl.grp1.sfspeed=115200 -F \"start_tracing=Start Tracing\"",
          null,
          "start_tracing was successful",
          "start tracing"
      );
    }
    public String toString() {
      return (String) this.getValue(Action.NAME);
    };
  };
  
  private Action traceStop = new AbstractAction("Stop tracing") {
    private static final long serialVersionUID = 1L;
    public void actionPerformed(ActionEvent e) {
      twistCommand(
          webUsernameField.getText(),
          new String(webPasswordField.getPassword()),
          serverField.getText(),
          "-F ctrl.grp1.sfversion=2 -F ctrl.grp1.sfspeed=115200 -F \"stop_tracing=Stop Tracing\"",
          null,
          "stop_tracing was successful",
          "stop tracing"
      );
    }
    public String toString() {
      return (String) this.getValue(Action.NAME);
    };
  };  
  
  private List<Mote> getTwistMotes() {
    List<Mote> twistMotes = new ArrayList<Mote>();
    for (final Mote m: simulation.getMotes()) {
      if (!(m instanceof TwistMoteType.TwistMote)) {
        continue;
      }
      TwistMoteType.TwistSerialMoteInterface s = m.getInterfaces().getInterfaceOfType(TwistMoteType.TwistSerialMoteInterface.class);
      if (s != null) {
        twistMotes.add(m);
      }
    }
    return twistMotes;
  }
    
  private void connectAll() {
    new Thread() {
      public void run() {
        for (final Mote m: getTwistMotes()) {
          TwistMoteType.TwistSerialMoteInterface s = m.getInterfaces().getInterfaceOfType(TwistMoteType.TwistSerialMoteInterface.class);
          if (s.isConnected()) {
            continue;
          }
          try {
            logger.info("Connecting mote " + m);
            s.connect(
                serverField.getText(),
                usernameField.getText(),
                new String(passwordField.getPassword()),
                resurrectBox.isSelected()
                );
          } catch (IOException e) {
            logger.warn("Error connect mote " + m + ": " + e.getMessage(), e);
          }
          updateTitle();
        }
      }
    }.start();
  }

  private void disconnectAll() {
    for (Mote m: getTwistMotes()) {
      TwistMoteType.TwistSerialMoteInterface s = m.getInterfaces().getInterfaceOfType(TwistMoteType.TwistSerialMoteInterface.class);
      s.disconnect();
      updateTitle();
    }
  } 
  
  private static int login(JDialog progressDialog, MessageList ml, File dir, String username, String password, String server) throws Exception {
    /* Authenticate */
    CompileContiki.compile(
        "curl -v -L -k " + curlAguments + " -d \"username=" + username + "\" -d \"password=" + password + "\" -d \"commit=Sign in\" " + server + "/__login__",
        null,
        null,
        dir,
        null,
        null,
        ml,
        true
    );
    boolean loginOK = false;
    for (MessageContainer m: ml.getMessages()) {
      if (m.message.contains("Welcome to TWIST")) {
        loginOK = true;
        break;
      }
    }

    if (!loginOK) {
      throw new Exception("Login failed for " + username + "/" + "********.");
    }

    /* Fetch job ID */
    CompileContiki.compile(
        "curl -v -L -k " + curlAguments + " " + server + "/jobs", 
        null,
        null,
        dir,
        null,
        null,
        ml,
        true
    );
    int jobID = -1;
    for (MessageContainer m: ml.getMessages()) {
      if (m.message.contains("<td class=\"active own\">")) {
        try {
          /* XXX Nasty code */
          jobID = Integer.parseInt(m.message.split("name=\"id_")[1].split("\"")[0]);
        } catch (RuntimeException e) {
          break;
        }
        break;
      }
    }
    if (jobID < 0) {
      throw new Exception("Job ID parsing failed: Unknown TWIST response");
    }
    return jobID;
  }
  
  private void twistCommand(final String username, final String password, final String server,
    final String webAction, final File directory, final String successString, final String actionName) {
    
    /* Set up a window showing the status of ongoing operations */
    final MessageList ml;
    final JDialog progressDialog;
    final JProgressBar progressBar;
    final JTextArea currStatus;
    final JButton closeButton;
    
    ml = new MessageList();
    progressDialog = new JDialog((Window)GUI.getTopParentContainer(), (String) null);
    progressDialog.setDefaultCloseOperation(JDialog.DISPOSE_ON_CLOSE);
    progressDialog.setSize(600, 500);
    progressDialog.setLocationRelativeTo(TwistPlugin.this);
    progressDialog.setVisible(false);
    
    progressBar = new JProgressBar(0, 100);
    progressBar.setValue(0);
    progressBar.setIndeterminate(true);
    
    JPanel progressPanel = new JPanel(new BorderLayout());
    progressPanel.add(BorderLayout.CENTER, new JScrollPane(ml));
    progressPanel.setBorder(BorderFactory.createEmptyBorder(10, 10, 10, 10));
    progressPanel.setVisible(true);
        
    Box controlBox = Box.createHorizontalBox();
    
    currStatus = new JTextArea();
    currStatus.setEditable(false);
    controlBox.add(currStatus);
    
    closeButton = new JButton("Close");
    closeButton.setVisible(false);
    closeButton.addActionListener(new ActionListener() {
      public void actionPerformed(ActionEvent e) {
        progressDialog.setVisible(false);
      }
    });
    closeButton.setVisible(false);
    controlBox.add(closeButton);
    
    progressDialog.getContentPane().add(BorderLayout.NORTH, progressBar);
    progressDialog.getContentPane().add(progressPanel);
    progressDialog.getContentPane().add(BorderLayout.SOUTH, controlBox);
    progressDialog.setTitle("Executing operation " + actionName);
    progressDialog.setVisible(true);
        
    new Thread() {
      public void run() {
        try {
          File dir = directory;
          if(dir == null) { /* Default dir is . */
            File tmp = File.createTempFile("tmp", "tmp");
            dir = tmp.getParentFile();
          }
          
          /* Log in and get id of the active job */
          currStatus.setText("Operation " + actionName + ": loging in user " + username);
          int jobID = login(progressDialog, ml, dir, username, password, server);
          
          /* Get mote list */
          String motes = "";
          for (Mote m: simulation.getMotes()) {
            if (!(m instanceof TwistMoteType.TwistMote)) {
              continue;
            }
            motes += m.getID() + " ";
          }
          motes = motes.trim();

          String strCommand = "";
          //if "nodes=" is present, it means we have assigned groups manually and we don't reassign
          if(webAction.contains("nodes=")) {
            strCommand += "curl -v -k " + curlAguments + " -F __nevow_form__=controlJob -F job_id=" + jobID + " " + webAction + " " + server + "/jobs/control";
          }
          else{  
            strCommand += "curl -v -k " + curlAguments + " -F __nevow_form__=controlJob -F job_id=" + jobID + " -F \"ctrl.grp1.nodes=" + motes + "\" " + webAction + " " + server + "/jobs/control";
          }

          /* Execute operation */
          currStatus.setText("Operation " + actionName + ": executing");
          CompileContiki.compile(
              "curl -v -k " + curlAguments + " -F __nevow_form__=controlJob -F job_id=" + jobID + " -F \"ctrl.grp1.nodes=" + motes + "\" " + webAction + " " + server + "/jobs/control",
              null,
              null,
              dir,
              null,
              null,
              ml,
              true
          );
          
          /* Look for successString in the HTTP response to check for success */
          boolean success;
          if(successString != null) {
            success = false;
            for (MessageContainer m: ml.getMessages()) {
              if (m.message.contains(successString)) {
                success = true;
              }
            }
          } else {
            success = true; 
          }
          
          closeButton.setVisible(true);
          /* Update GUI for success */
          if (success) {
            currStatus.setText("Operation " + actionName + ": success");
          } else {
            throw new Exception("Operation " + actionName + " failed");
          }
        } catch (Exception e1) {
          closeButton.setVisible(true);
          progressBar.setIndeterminate(false);
          currStatus.setText(e1.getMessage());
        }
      };
    }.start();
  }
    
  public Collection<Element> getConfigXML() {
    ArrayList<Element> config = new ArrayList<Element>();
    Element element;

    element = new Element("server");
    element.setText(serverField.getText());
    config.add(element);

    element = new Element("username");
    element.setText(usernameField.getText());
    config.add(element);

    element = new Element("webusername");
    element.setText(webUsernameField.getText());
    config.add(element);

    if (savePasswordsBox.isSelected()) {
      element = new Element("savepassword");
      config.add(element);

      element = new Element("password");
      element.setText(new String(passwordField.getPassword()));
      config.add(element);

      element = new Element("webpassword");
      element.setText(new String(webPasswordField.getPassword()));
      config.add(element);
    }

    if (autoconnectBox.isSelected()) {
      element = new Element("autoconnect");
      config.add(element);
    }
    if (resurrectBox.isSelected()) {
      element = new Element("resurrect");
      config.add(element);
    }

    return config;
  }

  public boolean setConfigXML(Collection<Element> configXML, boolean visAvailable) {
    for (Element element: configXML) {
      String name = element.getName();

      if (name.equals("server")) {
        serverField.setText(element.getText());
      }
      if (name.equals("username")) {
        usernameField.setText(element.getText());
      }
      if (name.equals("webusername")) {
        webUsernameField.setText(element.getText());
      }
      if (name.equals("password")) {
        passwordField.setText(element.getText());
      }
      if (name.equals("webpassword")) {
        webPasswordField.setText(element.getText());
      }
      if (name.equals("savepassword")) {
        savePasswordsBox.setSelected(true);
      }
      if (name.equals("resurrect")) {
        resurrectBox.setSelected(true);
      }
      if (name.equals("autoconnect")) {
        autoconnectBox.setSelected(true);

        /* Autoconnect */
        connectAll();
      }
    }
    return true;
  }

  private String getMoteIdsFromBinary(String binaryPath)
  {
    String motes = "";
    for(int row=0; row<tblSelectBinary.getRowCount(); row++) {
      if( (tblSelectBinary.getModel().getValueAt(row, COLUMN_BINPATH) == binaryPath) && (tblSelectBinary.getModel().getValueAt(row, COLUMN_ENABLED) == Boolean.TRUE) ) {
        motes+=tblSelectBinary.getModel().getValueAt(row, 0) + " ";
      }
    }
    return motes.trim();
  }

  private void addBinary(String binaryPath) {
    binairiesList.add(binaryPath);
    cbxBinTable.addItem(binaryPath);
    if (binairiesList.size() == 1) {
      cbxBinTable.setSelectedItem(binaryPath);
      applyBinToAll(binaryPath);
    }
  }
  
  private void applyBinToAll(String binaryPath) {
    tblSelectBinary.clearSelection();
    for (int i = 0; i < tblSelectBinary.getRowCount(); i++) {	
      tblSelectBinary.setValueAt(binaryPath, i, COLUMN_BINPATH);
    }
  }

  public void closePlugin() {
    simulation.getEventCentral().removeLogOutputListener(newMotesListener);
    for (Mote m: simulation.getMotes()) {
      newMotesListener.moteWasRemoved(m);
    }
    GUI.getTopParentContainer().repaint();
  }

  class BinTableCellRenderer implements TableCellRenderer {

    DefaultTableCellRenderer tableRenderer = new DefaultTableCellRenderer();
    public Component getTableCellRendererComponent(JTable table, Object value, boolean isSelected, boolean hasFocus, int row, int column) {
      tableRenderer = (DefaultTableCellRenderer) tableRenderer.getTableCellRendererComponent(table, value, isSelected, hasFocus, row, column);
      if(value!=null) {
        tableRenderer.setText(new File((String)value).getName());
      }
	    
      return tableRenderer;
    }
  }

}
