import java.awt.BorderLayout;
import java.awt.Color;
import java.awt.Dimension;
import java.awt.Font;
import java.awt.FontMetrics;
import java.awt.Frame;
import java.awt.GridLayout;
import java.awt.Image;
import java.awt.MediaTracker;
import java.awt.Rectangle;
import java.awt.Toolkit;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import java.awt.event.KeyEvent;
import java.awt.event.WindowAdapter;
import java.awt.event.WindowEvent;
import java.awt.event.WindowListener;
import java.io.BufferedWriter;
import java.io.File;
import java.io.FileOutputStream;
import java.io.OutputStreamWriter;
import java.util.ArrayList;
import java.util.Collection;
import java.util.Date;
import java.util.HashMap;
import java.util.Observable;
import java.util.Observer;
import java.util.Random;

import javax.swing.AbstractAction;
import javax.swing.Action;
import javax.swing.BorderFactory;
import javax.swing.DefaultCellEditor;
import javax.swing.Icon;
import javax.swing.ImageIcon;
import javax.swing.InputMap;
import javax.swing.JButton;
import javax.swing.JComboBox;
import javax.swing.JComponent;
import javax.swing.JDialog;
import javax.swing.JPanel;
import javax.swing.JScrollPane;
import javax.swing.JSplitPane;
import javax.swing.JTable;
import javax.swing.JTextArea;
import javax.swing.KeyStroke;
import javax.swing.ListSelectionModel;
import javax.swing.SwingUtilities;
import javax.swing.table.AbstractTableModel;
import javax.swing.table.DefaultTableCellRenderer;
import javax.swing.table.TableCellEditor;

import org.apache.log4j.Logger;
import org.jdom.Element;
import org.json.simple.JSONArray;
import org.json.simple.JSONObject;
import org.json.simple.JSONValue;

import se.sics.cooja.ClassDescription;
import se.sics.cooja.GUI;
import se.sics.cooja.Mote;
import se.sics.cooja.MoteType;
import se.sics.cooja.MoteType.MoteTypeCreationException;
import se.sics.cooja.PluginType;
import se.sics.cooja.SectionMoteMemory;
import se.sics.cooja.Simulation;
import se.sics.cooja.TimeEvent;
import se.sics.cooja.VisPlugin;
import se.sics.cooja.contikimote.ContikiMote;
import se.sics.cooja.contikimote.ContikiMoteType;
import se.sics.cooja.dialogs.CompileContiki;
import se.sics.cooja.dialogs.MessageList;
import se.sics.cooja.dialogs.MessageList.MessageContainer;
import se.sics.cooja.dialogs.TableColumnAdjuster;
import se.sics.cooja.radiomediums.DestinationRadio;
import se.sics.cooja.radiomediums.DirectedGraphMedium;
import se.sics.cooja.util.StringUtils;

@ClassDescription("COOJA/KleeNet")
@PluginType(PluginType.SIM_PLUGIN)
public class Prepare extends VisPlugin {
	private static final long serialVersionUID = 4571578141658600722L;
	private static Logger logger = Logger.getLogger(Prepare.class);

	private Simulation sim;
	private Observer simObserver;
	
	private Icon iconOK =
		new ImageIcon(this.getClass().getClassLoader().getResource("ok.png"));
	private Icon iconFail =
		new ImageIcon(this.getClass().getClassLoader().getResource("fail.png"));
	private Icon iconPending =
		new ImageIcon(this.getClass().getClassLoader().getResource("pending.png"));

	/**
	 * @param simulation Simulation object
	 * @param gui GUI object 
	 */
	public Prepare(Simulation simulation, GUI gui) {
		super("COOJA/KleeNet", gui);
		this.sim = simulation;

		/* Set workload dependencies */
		cleanAction.setDependencies(null, checkAction);
		checkAction.setDependencies(cleanAction, configureAction);
		configureAction.setDependencies(checkAction, exportAction);
		exportAction.setDependencies(configureAction, compileAction);
		compileAction.setDependencies(exportAction, runAction);
		runAction.setDependencies(compileAction, simulateAction);
		/* XXX: if KleeNet wasn't run, take the concrete values from the last run */
		//simulateAction.setDependencies(runAction, null);

		/* Listen for simulation changes */
		sim.addObserver(simObserver = new Observer() {
			public void update(Observable o, Object arg) {
				cleanAction.reset();
			}
		});

		/* Layout */
		JPanel contentPane = new JPanel(new BorderLayout());

		JPanel centerGrid = new JPanel(new GridLayout(7, 1));
		centerGrid.add(cleanButton);
		centerGrid.add(checkButton);
		centerGrid.add(configureButton);
		centerGrid.add(exportButton);
		centerGrid.add(compileButton);
		centerGrid.add(runButton);
		centerGrid.add(simulateButton);
		centerGrid.setMinimumSize(new Dimension(200, 6*35));
		centerGrid.setPreferredSize(new Dimension(200, 6*35));
		contentPane.add(BorderLayout.CENTER, centerGrid);

		contentPane.setBorder(BorderFactory.createEmptyBorder(5, 5, 5, 5));
		setContentPane(contentPane);
		pack();
	}

	public void closePlugin() {
		sim.deleteObserver(simObserver);
	}

	private File workingDir = null;
	private void clean(MessageList output) throws Exception {
		workingDir = sim.getMoteTypes()[0].getContikiSourceFile().getParentFile();
		output.addMessage("### Working directory: " + workingDir);

		/* make clean */
		try {
			CompileContiki.compile(
					"make TARGET=coojakleenet clean",
					null,
					null,
					workingDir,
					null,
					null,
					output,
					true
			);
		} catch (Exception e) {
			MoteTypeCreationException e2 =
				new MoteTypeCreationException("Command failed: " + e.getMessage());
			e2 = (MoteTypeCreationException) e2.initCause(e);
			e2.setCompilationOutput(output);

			/* Print last 10 compilation errors to console */
			MessageContainer[] messages = output.getMessages();
			for (int i=messages.length-10; i < messages.length; i++) {
				if (i < 0) {
					continue;
				}
				logger.fatal(">> " + messages[i]);
			}

			logger.fatal("Clean command error: " + e.getMessage());
			throw e2;
		}
	}

	private void check(MessageList output) throws Exception {
		/* Motes and mote types */
		if (sim.getMoteTypes().length == 0) {
			throw new Exception("Unsupported number of mote types: " + sim.getMoteTypes().length);
		}
		for (MoteType mt: sim.getMoteTypes()) {
			if (!(mt instanceof ContikiMoteType)) {
				throw new Exception("Unsupported mote type: " + GUI.getDescriptionOf(mt));
			}
			if (!mt.getContikiSourceFile().getParentFile().equals(workingDir)) {
				throw new Exception("All Contiki application must reside in the same directory!");
			}
		}
		output.addMessage("Mote types: OK");
		if (sim.getMotes().length == 0) {
			throw new Exception("No simulated motes");
		}
		output.addMessage("Motes: OK");

		/* Radio medium */
		if (!(sim.getRadioMedium() instanceof DirectedGraphMedium)) {
			throw new Exception("Bad radio medium: " + GUI.getDescriptionOf(sim.getRadioMedium()));
		}
		output.addMessage("Radio medium: OK");

		/* Simulation state */
		if (sim.getSimulationTime() > 0) {
			throw new Exception("Simulation must have simulation time 0! Reload simulation.");
		}
		output.addMessage("Simulation state: OK");
	}

	private String moteObjectFiles = null;
	private void export(MessageList output) throws Exception {
		/* Prepare MOTES env variable (for Makefile.coojakleenet) */
		moteObjectFiles = null;
		for (Mote m: sim.getMotes()) {
			String moteObject = m.getType().getContikiSourceFile().getName();
			moteObject = moteObject.substring(0, moteObject.length()-2); /* strip .c */
			moteObject += "." + m.getID() + ".coojakleenet-mote.bc";

			if (moteObjectFiles != null) {
				moteObjectFiles += "," + moteObject;
			} else {
				moteObjectFiles = moteObject;
			}
		}
		output.addMessage("### KleeNet motes: " + moteObjectFiles);

		/* Generate network_driver_conf.h */
		File netConf = new File(workingDir, "network_driver_conf.h");
		if (netConf.exists()) {
			output.addMessage("### Removing previous autogenerated configuration: " + netConf.getName());
			netConf.delete();
		}
		output.addMessage("### Generating network configuration header: " + netConf.getName());
		BufferedWriter netConfWriter = new BufferedWriter(new OutputStreamWriter(new FileOutputStream(netConf)));
		netConfWriter.write("/* " + new Date().toString() + ". " + netConf.getName() + " autogenerated from COOJA. */\n");
		netConfWriter.write("#define ALL_MOTE_EXTERNS() ");
		for (Mote m: sim.getMotes()) {
			netConfWriter.write("MOTE_EXTERNS(" + m.getID() + "); ");
		}
		netConfWriter.write("\n");
		netConfWriter.write("#define ALL_MOTE_CASES() ");
		for (Mote m: sim.getMotes()) {
			int drops;
			if (motePacketDrops.containsKey(m)) {
				drops = motePacketDrops.get(m);
			} else {
				drops = 0;
			}
			int subsequentDrops;
			if (motePacketSubsequentDrops.containsKey(m)) {
				subsequentDrops = motePacketSubsequentDrops.get(m);
			} else {
				subsequentDrops = 0;
			}
			int duplicates;
			if (motePacketDuplicates.containsKey(m)) {
				duplicates = motePacketDuplicates.get(m);
			} else {
				duplicates = 0;
			}
			int reboots;
			if (moteReboots.containsKey(m)) {
				reboots = moteReboots.get(m);
			} else {
				reboots = 0;
			}
			int outages;
			if (moteOutages.containsKey(m)) {
				outages = moteOutages.get(m);
			} else {
				outages = 0;
			}
			int failureDelay;
			if (moteFailureDelays.containsKey(m)) {
				failureDelay = moteFailureDelays.get(m);
			} else {
				failureDelay = 0;
			}

			netConfWriter.write("MOTE_CASE(" +
					/* mode id */ + m.getID() + ", " +
					/* random seed */ + m.getSimulation().getRandomSeed() + ", "
					/* clock drift */ + (int)m.getInterfaces().getClock().getDrift() + ", " +
					/* drops */ drops + ", " + 
					/* subsequent drops */ subsequentDrops + ", " + 
					/* duplicates */ duplicates + ", " + 
					/* reboots */ reboots + ", " +  
					/* outages */ outages + ", " +  
					/* failure delay */ failureDelay + "); ");
		}
		netConfWriter.write("\n");
		netConfWriter.write("#define ALL_IN_BUFFERS() in_buffer * const in_buffers[] = { 0");
		for (Mote m: sim.getMotes()) {
			netConfWriter.write(", &mote" + m.getID() + "_in_buffer");
		}
		netConfWriter.write("};\n");

		{
			DirectedGraphMedium radioMedium = (DirectedGraphMedium) sim.getRadioMedium();
			String dgrmFrom = "{";
			String dgrmTo = "{";
			int nrLinks = 0;
			for (Mote m: sim.getMotes()) {
				DestinationRadio[] allDests = radioMedium.getPotentialDestinations(m.getInterfaces().getRadio());
				if (allDests != null) {
					for (DestinationRadio r: allDests) {
						dgrmFrom += m.getID() + ",";
						dgrmTo += r.radio.getMote().getID() + ",";
						nrLinks++;
					}
				}
			}
			dgrmFrom = dgrmFrom.substring(0, dgrmFrom.length()-1) + "}";
			dgrmTo = dgrmTo.substring(0, dgrmTo.length()-1) + "}";
			if (nrLinks == 0 || dgrmTo.length() < 3 || dgrmFrom.length() < 3 ) {
				output.addMessage("### DGRM Export error: No links", MessageList.ERROR);
				throw new RuntimeException("DGRM Export error: No links");
			} else {
				netConfWriter.write("\n");
				netConfWriter.write("#define DGRM_NR_LINKS " + nrLinks + "\n");
				netConfWriter.write("#define DGRM_LINKS_FROM() " + dgrmFrom + "\n");
				netConfWriter.write("#define DGRM_LINKS_TO() " + dgrmTo + "\n");
			}
		}
		netConfWriter.close();
	}

	private File kleenetExecutable = null;
	private void compile(MessageList output) throws Exception {
		/* Compile KleeNet executable */
		kleenetExecutable = new File(workingDir, "sim" + new Random().nextInt(100) + ".coojakleenet-executable.bc");
		try {
			CompileContiki.compile(
					"make TARGET=coojakleenet " + kleenetExecutable.getName() + " MOTES=" + moteObjectFiles,
					null,
					null,
					workingDir,
					null,
					null,
					output,
					true
			);
		} catch (Exception e) {
			MoteTypeCreationException e2 =
				new MoteTypeCreationException("Compilation failed: " + e.getMessage());
			e2 = (MoteTypeCreationException) e2.initCause(e);
			e2.setCompilationOutput(output);

			/* Print last 10 compilation errors to console */
			MessageContainer[] messages = output.getMessages();
			for (int i=messages.length-10; i < messages.length; i++) {
				if (i < 0) {
					continue;
				}
				logger.fatal(">> " + messages[i]);
			}

			logger.fatal("Compilation error: " + e.getMessage());
			throw e2;
		}
		output.addMessage("### Compilation success: " + kleenetExecutable);
	}

	private Process kleenetProcess = null;
	private String kleeOutputDirectory = null;
	private void runKleenet(MessageList output) throws Exception {
		/* Compile KleeNet executable */
		String target = kleenetExecutable.getName().replace("coojakleenet-executable.bc", "coojakleenet-run");
		Process process;

		/* Guess next KleeNet output directory */
		int counter = 0;
		while (true) {
			File outDir = new File(workingDir, "klee-out-" + counter);
			if (outDir.exists() && outDir.isDirectory()) {
				counter++;
			} else {
				kleeOutputDirectory = outDir.getName();
				break;
			}
		}

		try {
			process = CompileContiki.compile(
					"make TARGET=coojakleenet " + target,
					null,
					null,
					workingDir,
					null,
					null,
					output,
					false
			);
		} catch (Exception e) {
			MoteTypeCreationException e2 =
				new MoteTypeCreationException("Kleenet export failed: " + e.getMessage());
			e2 = (MoteTypeCreationException) e2.initCause(e);
			e2.setCompilationOutput(output);

			/* Print last 10 compilation errors to console */
			MessageContainer[] messages = output.getMessages();
			for (int i=messages.length-10; i < messages.length; i++) {
				if (i < 0) {
					continue;
				}
				logger.fatal(">> " + messages[i]);
			}

			logger.fatal("Compilation error: " + e.getMessage());
			throw e2;
		}
		output.addMessage("### KleeNet started");

		kleenetProcess = process;
		process.waitFor();
		kleenetProcess = null;
		if (process.exitValue() != 0) {
			throw new Exception("Bad process exit value: " + process.exitValue());
		}
	}

	private static enum State {
		PENDING,
		OK,
		FAILED,
		UNKNOWN
	};
	private static Rectangle lastWorkloadCommandOutputBounds = null;
	abstract class Workload extends AbstractAction {
		private static final long serialVersionUID = 5464186289230442762L;

		private State state = State.UNKNOWN;
		protected Workload parent = null;
		protected Workload child = null;

		public Workload(String desc) {
			super(desc);
		}

		public void setDependencies(Workload parent, Workload child) {
			this.parent = parent;
			this.child = child;
		}

		public void actionPerformed(ActionEvent e) {
			reset();

			(new Thread() {
				public void run() {
					try {
						Workload.this.run();
					} catch (Exception e) {
						logger.fatal("Error: ", e);
						if (commandOutput != null) {
							e.printStackTrace(commandOutput.getInputStream(MessageList.ERROR));
						}
					}
				};
			}).start();
		}

		public boolean isReady() {
			return state == State.OK;
		}

		public void reset() {
			state = State.UNKNOWN;
			putValue(Action.LARGE_ICON_KEY, null);

			if (child != null) {
				child.reset();
			}
		}

		public final void run() throws Exception {
			if (isReady()) {
				return;
			}

			if (parent != null) {
				try {
					parent.run();
				} catch (Exception e) {
					if (parent.commandOutput != null) {
						e.printStackTrace(parent.commandOutput.getInputStream(MessageList.ERROR));
					}
					throw e;
				}
			}

			state = State.PENDING;
			putValue(Action.LARGE_ICON_KEY, iconPending);
			try {
				work();
			} catch (Exception ex) {
				state = State.FAILED;
				putValue(Action.LARGE_ICON_KEY, iconFail);
				throw ex;
			}
			state = State.OK;
			putValue(Action.LARGE_ICON_KEY, iconOK);
		}

		protected abstract void work() throws Exception;

		protected Thread workerThread = null;
		protected JDialog dialog = null;
		protected MessageList commandOutput = null;
		protected void setupCommandOutputDialog(String title) throws Exception {
			workerThread = Thread.currentThread();
			dialog = new JDialog((Frame) GUI.getTopParentContainer(), title, false);
			final JDialog dialogCopy = dialog;
			commandOutput = new MessageList();
			SwingUtilities.invokeAndWait(new Runnable() {
				public void run() {
					commandOutput.addPopupMenuItem(null, true);

					/* Close on escape, kill thread and kleenet process */
					final AbstractAction escapeAction = new AbstractAction() {
						private static final long serialVersionUID = -7926917353240187344L;
						public void actionPerformed(ActionEvent e) {
							for (WindowListener wl: dialogCopy.getWindowListeners()) {
								wl.windowClosing(null);
							}
						}
					};
					dialog.getRootPane().getInputMap(JComponent.WHEN_ANCESTOR_OF_FOCUSED_COMPONENT).
					put(KeyStroke.getKeyStroke(KeyEvent.VK_ESCAPE, 0, false), "escape");
					dialog.getRootPane().getActionMap().put("escape", escapeAction);
					dialog.setDefaultCloseOperation(JDialog.DO_NOTHING_ON_CLOSE);
					dialog.addWindowListener(new WindowAdapter() {
						private void killProcess() {
							if (kleenetProcess != null) {
								kleenetProcess.destroy();
								kleenetProcess = null;
							}

							if (workerThread != null && workerThread.isAlive()) {
								workerThread.interrupt();
							}

							if (dialogCopy.isVisible()) {
								lastWorkloadCommandOutputBounds = dialogCopy.getBounds();
								dialogCopy.dispose();
							}
						}
						public void windowClosing(WindowEvent e) {
							if (kleenetProcess == null) {
								killProcess();
								return;
							}

							/* Try to exit cleanly via klee-control */
							try {
								Thread t = new Thread() {
									public void run() {
										logger.info("Requesting clean shutdown of KleeNet via pkill");
										try {
											CompileContiki.compile(
													"pkill kleenet",
													null,
													null,
													workingDir,
													null,
													null,
													commandOutput,
													true
											);
											dialog.setTitle("Running (requested shutdown)");
											kleenetProcess.waitFor();
											if (dialogCopy.isVisible()) {
												lastWorkloadCommandOutputBounds = dialogCopy.getBounds();
												dialogCopy.dispose();
											}

										} catch (Exception e) {
											logger.warn("Failed clean shutdown of KleeNet, killing process: " + e.getMessage());
											killProcess();
										}
									}
								};
								t.start();
							} catch (Exception e1) {
								logger.warn("Failed clean shutdown of KleeNet, killing process: " + e1.getMessage());
								killProcess();
							}
						}
					});

					/* Layout */
					JSplitPane splitPane = new JSplitPane(JSplitPane.HORIZONTAL_SPLIT);
					splitPane.setLeftComponent(new JScrollPane(commandOutput));
					dialog.setContentPane(splitPane);
					if (lastWorkloadCommandOutputBounds != null) {
						dialog.setBounds(lastWorkloadCommandOutputBounds);
					} else {
						dialog.setSize(700, 500);
						dialog.setLocationRelativeTo(Prepare.this);
						dialog.setLocation(dialog.getLocation().x, dialog.getLocation().y+230);
					}
					dialog.setVisible(true);
				}
			});
		}
		protected void disposeCommandOutputDialog() throws Exception {
			SwingUtilities.invokeAndWait(new Runnable() {
				public void run() {
					lastWorkloadCommandOutputBounds = dialog.getBounds();
					dialog.dispose();
				}
			});
		}
	}

	private Workload cleanAction = new Workload("Clean") {
		private static final long serialVersionUID = 3989900297112209979L;

		public void work() throws Exception {
			/* Setup dialog */
			setupCommandOutputDialog("Cleaning");

			clean(commandOutput);
			Thread.sleep(500);

			/* Dispose dialog */
			disposeCommandOutputDialog();
		};
	};
	private JButton cleanButton = new JButton(cleanAction);

	private HashMap<Mote,Integer> motePacketDrops = new HashMap<Mote,Integer>();
	private HashMap<Mote,Integer> motePacketSubsequentDrops = new HashMap<Mote,Integer>();
	private HashMap<Mote,Integer> motePacketDuplicates = new HashMap<Mote,Integer>();
	private HashMap<Mote,Integer> moteReboots = new HashMap<Mote,Integer>();
	private HashMap<Mote,Integer> moteOutages = new HashMap<Mote,Integer>();
	private HashMap<Mote,Integer> moteFailureDelays = new HashMap<Mote,Integer>();
	private Workload configureAction = new Workload("Configure") {
		private static final long serialVersionUID = 8582732650445989871L;

		private boolean ok = false;
		private final static int IDX_MOTE = 0;
		private final static int IDX_PACKETDROPS = 1;
		private final static int IDX_PACKETSUBSEQUENTDROPS = 2;
		private final static int IDX_PACKETDUPLICATES = 3;
		private final static int IDX_REBOOTS = 4;
		private final static int IDX_OUTAGES = 5;
		private final static int IDX_DELAY = 6;
		private final String[] columns = new String[] {
				"Mote", "Packet drops", "Subsequent packet drops", "Packet Duplicates", "Reboots (10s)", "Outages (10s)", "Failure delay (s)"
		};
		private JComboBox combobox = new JComboBox();
		final AbstractTableModel model = new AbstractTableModel() {
			private static final long serialVersionUID = -2950758852628870034L;
			public String getColumnName(int column) {
				if (column < 0 || column >= columns.length) {
					logger.fatal("Unknown column: " + column);
					return "";
				}
				return columns[column];
			}
			public int getRowCount() {
				return sim.getMotesCount();
			}
			public int getColumnCount() {
				return columns.length;
			}
			public Object getValueAt(int row, int column) {
				if (row < 0 || row >= sim.getMotesCount()) {
					return "";
				}
				Mote mote = sim.getMote(row);
				if (column == IDX_MOTE) {
					return "" + sim.getMote(row);
				}
				if (column == IDX_PACKETDROPS) {
					if (motePacketDrops.containsKey(mote)) {
						return "" + motePacketDrops.get(mote);
					} else {
						return "" + 0;
					}
				}
				if (column == IDX_PACKETSUBSEQUENTDROPS) {
					if (motePacketSubsequentDrops.containsKey(mote)) {
						return "" + motePacketSubsequentDrops.get(mote);
					} else {
						return "" + 0;
					}
				}
				if (column == IDX_PACKETDUPLICATES) {
					if (motePacketDuplicates.containsKey(mote)) {
						return "" + motePacketDuplicates.get(mote);
					} else {
						return "" + 0;
					}
				}
				if (column == IDX_REBOOTS) {
					if (moteReboots.containsKey(mote)) {
						return "" + moteReboots.get(mote);
					} else {
						return "" + 0;
					}
				}
				if (column == IDX_OUTAGES) {
					if (moteOutages.containsKey(mote)) {
						return "" + moteOutages.get(mote);
					} else {
						return "" + 0;
					}
				}
				if (column == IDX_DELAY) {
					if (moteFailureDelays.containsKey(mote)) {
						return "" + moteFailureDelays.get(mote);
					} else {
						return "" + 0;
					}
				}
				return "";
			}
			public void setValueAt(Object value, int row, int column) {
				if (row < 0 || row >= sim.getMotesCount()) {
					return;
				}
				Mote mote = sim.getMote(row);
				if (column == IDX_PACKETDROPS) {
					try {
						motePacketDrops.put(mote, ((Number)value).intValue());
					} catch (ClassCastException e) {
					}
					return;
				}
				if (column == IDX_PACKETSUBSEQUENTDROPS) {
					try {
						motePacketSubsequentDrops.put(mote, ((Number)value).intValue());
					} catch (ClassCastException e) {
					}
					return;
				}
				if (column == IDX_PACKETDUPLICATES) {
					try {
						motePacketDuplicates.put(mote, ((Number)value).intValue());
					} catch (ClassCastException e) {
					}
					return;
				}
				if (column == IDX_REBOOTS) {
					try {
						moteReboots.put(mote, ((Number)value).intValue());
					} catch (ClassCastException e) {
					}
					return;
				}
				if (column == IDX_OUTAGES) {
					try {
						moteOutages.put(mote, ((Number)value).intValue());
					} catch (ClassCastException e) {
					}
					return;
				}
				if (column == IDX_DELAY) {
					moteFailureDelays.put(mote, ((Number)value).intValue());
					return;
				}
				super.setValueAt(value, row, column);
			}
			public boolean isCellEditable(int row, int column) {
				if (row < 0 || row >= sim.getMotesCount()) {
					return false;
				}

				Mote mote = sim.getMote(row);
				if (column == IDX_MOTE) {
					sim.getGUI().signalMoteHighlight(mote);
					return false;
				}
				if (column == IDX_PACKETDROPS) {
					return true;
				}
				if (column == IDX_PACKETSUBSEQUENTDROPS) {
					return true;
				}
				if (column == IDX_PACKETDUPLICATES) {
					return true;
				}
				if (column == IDX_REBOOTS) {
					return true;
				}
				if (column == IDX_OUTAGES) {
					return true;
				}
				if (column == IDX_DELAY) {
					return true;
				}
				return false;
			}
			public Class<?> getColumnClass(int c) {
				return getValueAt(0, c).getClass();
			}
		};

		public void work() throws Exception {
			JTable graphTable = new JTable(model) {
				private static final long serialVersionUID = -7166360766857450914L;
				public TableCellEditor getCellEditor(int row, int column) {
					if (column == IDX_PACKETDROPS) {
						combobox.removeAllItems();
						for (int i=0; i <= 100; i++) {
							combobox.addItem(i);
						}
					}
					if (column == IDX_PACKETSUBSEQUENTDROPS) {
						combobox.removeAllItems();
						for (int i=0; i <= 100; i++) {
							combobox.addItem(i);
						}
					}
					if (column == IDX_PACKETDUPLICATES) {
						combobox.removeAllItems();
						for (int i=0; i <= 100; i++) {
							combobox.addItem(i);
						}
					}
					if (column == IDX_REBOOTS) {
						combobox.removeAllItems();
						for (int i=0; i <= 100; i++) {
							combobox.addItem(i);
						}
					}
					if (column == IDX_OUTAGES) {
						combobox.removeAllItems();
						for (int i=0; i <= 100; i++) {
							combobox.addItem(i);
						}
					}
					if (column == IDX_DELAY) {
						combobox.removeAllItems();
						for (int i=0; i <= 100; i++) {
							combobox.addItem(i);
						}
					}
					return super.getCellEditor(row, column);
				}
			};

			/* TODO Perform below in AWT */

			/* Setup table dialog (blocking) */
			JButton button = new JButton("OK");
			button.addActionListener(new ActionListener() {
				public void actionPerformed(ActionEvent e) {
					ok = true;
					dialog.dispose();
				}
			});

			graphTable.getColumnModel().getColumn(IDX_PACKETDROPS).setCellRenderer(new DefaultTableCellRenderer() {
				private static final long serialVersionUID = 6328483552097801528L;
				public void setValue(Object value) {
					if (!(value instanceof Integer)) {
						setText(value.toString());
						return;
					}
					int v = ((Integer) value).intValue();
					setText("" + v);
				}
			});
			graphTable.getColumnModel().getColumn(IDX_PACKETSUBSEQUENTDROPS).setCellRenderer(new DefaultTableCellRenderer() {
				private static final long serialVersionUID = 6328483552097801529L;
				public void setValue(Object value) {
					if (!(value instanceof Integer)) {
						setText(value.toString());
						return;
					}
					int v = ((Integer) value).intValue();
					setText("" + v);
				}
			});
			graphTable.getColumnModel().getColumn(IDX_PACKETDUPLICATES).setCellRenderer(new DefaultTableCellRenderer() {
				private static final long serialVersionUID = 6328483662097801529L;
				public void setValue(Object value) {
					if (!(value instanceof Integer)) {
						setText(value.toString());
						return;
					}
					int v = ((Integer) value).intValue();
					setText("" + v);
				}
			});
			graphTable.getColumnModel().getColumn(IDX_REBOOTS).setCellRenderer(new DefaultTableCellRenderer() {
				private static final long serialVersionUID = 1832079671948040744L;

				public void setValue(Object value) {
					if (!(value instanceof Integer)) {
						setText(value.toString());
						return;
					}
					int v = ((Integer) value).intValue();
					setText("" + v);
				}
			});
			graphTable.getColumnModel().getColumn(IDX_OUTAGES).setCellRenderer(new DefaultTableCellRenderer() {
				private static final long serialVersionUID = 1833479671948040744L;

				public void setValue(Object value) {
					if (!(value instanceof Integer)) {
						setText(value.toString());
						return;
					}
					int v = ((Integer) value).intValue();
					setText("" + v);
				}
			});
			graphTable.getColumnModel().getColumn(IDX_DELAY).setCellRenderer(new DefaultTableCellRenderer() {
				private static final long serialVersionUID = 808345927453114989L;
				public void setValue(Object value) {
					if (!(value instanceof Integer)) {
						setText(value.toString());
						return;
					}
					int v = ((Integer) value).intValue();
					setText(v + " ms");
				}
			});
			graphTable.getColumnModel().getColumn(IDX_PACKETDROPS).setCellEditor(new DefaultCellEditor(combobox));
			graphTable.getColumnModel().getColumn(IDX_PACKETSUBSEQUENTDROPS).setCellEditor(new DefaultCellEditor(combobox));
			graphTable.getColumnModel().getColumn(IDX_PACKETDUPLICATES).setCellEditor(new DefaultCellEditor(combobox));
			graphTable.getColumnModel().getColumn(IDX_REBOOTS).setCellEditor(new DefaultCellEditor(combobox));
			graphTable.getColumnModel().getColumn(IDX_OUTAGES).setCellEditor(new DefaultCellEditor(combobox));
			graphTable.getColumnModel().getColumn(IDX_DELAY).setCellEditor(new DefaultCellEditor(combobox));

			dialog = new JDialog((Frame) GUI.getTopParentContainer(), "Configure", true);

			InputMap inputMap = dialog.getRootPane().getInputMap(JComponent.WHEN_ANCESTOR_OF_FOCUSED_COMPONENT);
			inputMap.put(KeyStroke.getKeyStroke(KeyEvent.VK_ESCAPE, 0, false), "dispose");
			dialog.getRootPane().getActionMap().put("dispose", new AbstractAction() {
				private static final long serialVersionUID = -2369224678892370745L;
				public void actionPerformed(ActionEvent arg0) {
					dialog.dispose();
				}
			});

			dialog.getRootPane().setDefaultButton(button);
			dialog.getContentPane().setLayout(new BorderLayout());
			dialog.add(BorderLayout.CENTER, new JScrollPane(graphTable));
			dialog.add(BorderLayout.SOUTH, button);
			dialog.setSize(500,200);
			dialog.setLocationRelativeTo(Prepare.this);
			ok = false;
			dialog.setVisible(true); /* blocks */
			if (!ok) {
				throw new Exception("Configure dialog was cancelled by user");
			}
		};
	};
	private JButton configureButton = new JButton(configureAction);

	private Workload checkAction = new Workload("Check") {
		private static final long serialVersionUID = -3198274747096948800L;

		public void work() throws Exception {
			/* Setup dialog */
			setupCommandOutputDialog("Checking");

			check(commandOutput);
			Thread.sleep(500);

			/* Dispose dialog */
			disposeCommandOutputDialog();
		};
	};
	private JButton checkButton = new JButton(checkAction);

	private Workload exportAction = new Workload("Export") {
		private static final long serialVersionUID = 6979409970366308023L;

		public void work() throws Exception {
			/* Setup dialog */
			setupCommandOutputDialog("Exporting");

			export(commandOutput);
			Thread.sleep(500);

			/* Dispose dialog */
			disposeCommandOutputDialog();
		};
	};
	private JButton exportButton = new JButton(exportAction);

	private Workload compileAction = new Workload("Compile") {
		private static final long serialVersionUID = 7964400063143192035L;

		public void work() throws Exception {
			/* Setup dialog */
			setupCommandOutputDialog("Compiling");

			compile(commandOutput);
			Thread.sleep(500);

			/* Dispose dialog */
			disposeCommandOutputDialog();
		};
	};
	private JButton compileButton = new JButton(compileAction);

	private File testInfo = null;
	private Workload runAction = new Workload("Run") {
		private static final long serialVersionUID = 394169684506535065L;
		private Image treeImage = null;
		private JPanel panel = null;
		private String statsString = null;
		private String errorString = null;

		public void work() throws Exception {
			/* Setup command dialog */
			setupCommandOutputDialog("Running");

			/* Setup tree graph panel */
			final Thread generateTreeThread = new Thread() {
				public void run() {
					while (true) {
						/* Delay */
						try {
							Thread.sleep(1000); /* XXX Generation interval? */
						} catch (InterruptedException e) {
						}
						if (getName().equals("stop")) {
							break;
						}

						/* Generate image */
						if (kleeOutputDirectory == null) {
							logger.warn("Unknown KleeNet output directory, retrying");
							continue;
						}
						try {
							/* Read runtime statistics */
							try {
								String fileData = StringUtils.loadFromFile(new File(workingDir, kleeOutputDirectory + "/run.stats"));
								String[] arr = fileData.split("\n");
								String last = arr[arr.length-1];
								last = last.replace("(", "");
								last = last.replace(")", "");
								arr = last.split(",");
								String instructions = arr[0];
								String numStates = arr[5];

								statsString = instructions + " instructions, " + numStates + " states";

								/* Does *.assert.err exist? */
								File testInfo = new File(workingDir, kleeOutputDirectory);
								String[] allFiles = testInfo.list();
								for (String f: allFiles) {
									if (f.endsWith(".assert.err")) {
										errorString = "KLEENET ASSERTION!";
										break;
									}
								}

							} catch (Exception e2) {
								statsString = null;
								errorString = null;
							}

							CompileContiki.compile(
									"touch " + kleeOutputDirectory + "/symPaths.ts",
									null,
									null,
									workingDir,
									null,
									null,
									new MessageList() /* ignoring output */,
									true
							);
							CompileContiki.compile(
									"make TARGET=coojakleenet " + kleeOutputDirectory + "/symPaths.png",
									null,
									null,
									workingDir,
									null,
									null,
									new MessageList() /* ignoring output */,
									true
							);
						} catch (Exception e) {
						}

						/* Load image */
						File treeImageFile = new File(workingDir, kleeOutputDirectory + "/symPaths.png");
						if (!treeImageFile.exists()) {
							logger.warn("No tree graph image file, retrying");
							continue;
						}

						Toolkit toolkit = Toolkit.getDefaultToolkit();
						treeImage = toolkit.getImage(treeImageFile.getAbsolutePath());
						treeImage.flush();
						MediaTracker tracker = new MediaTracker(Prepare.this);
						tracker.addImage(treeImage, 1);
						try {
							tracker.waitForAll();
							if (tracker.isErrorAny()) {
								treeImage = null;
							}
						} catch (InterruptedException ex) {
						}

						if (treeImage == null) {
							logger.warn("Error loading image, retrying");
							continue;
						}

						if (panel != null) {
							panel.repaint();
							panel.revalidate();
						}
					}

					logger.info("Tree graph generation thread exited");
					treeImage = null;
				};
			};
			generateTreeThread.start();

			SwingUtilities.invokeAndWait(new Runnable() {
				public void run() {
					/* Layout */
					panel = new JPanel(new BorderLayout()) {
						private static final long serialVersionUID = 1L;
						protected void paintComponent(java.awt.Graphics g) {
							super.paintComponent(g);
							if (treeImage == null) {
								/* Pending text */
								g.setColor(Color.LIGHT_GRAY);
								g.fillRect(0, 0, getWidth(), getHeight());

								FontMetrics fm = g.getFontMetrics();
								g.setColor(Color.BLACK);
								int msgWidth = fm.stringWidth("[generating tree graph]");
								g.drawString("[generating tree graph]", getWidth()/2 - msgWidth/2, getHeight()/2 + 3);
								return;
							}

							g.drawImage(treeImage, 0, 0, getWidth(), getHeight(), null);
							if (statsString != null) {
								FontMetrics fm = g.getFontMetrics();
								g.setColor(Color.BLACK);
								int msgWidth = fm.stringWidth(statsString);
								g.drawString(statsString, getWidth()/2 - msgWidth/2, 15);
							}
							if (errorString != null) {
								FontMetrics fm = g.getFontMetrics();
								g.setColor(Color.BLACK);
								int msgWidth = fm.stringWidth(errorString);
								g.drawString(errorString, getWidth()/2 - msgWidth/2, getHeight()-10);
								g.setColor(new Color(1, 0, 0, 0.3f));
								g.fillRect(0, 0, getWidth(), getHeight());
							}
						};
					};

					panel.setBackground(Color.BLACK);
					panel.setPreferredSize(new Dimension(500, 500));
					JSplitPane splitPane = (JSplitPane) dialog.getContentPane();
					splitPane.setRightComponent(panel);
					splitPane.setDividerLocation(0.4);
					dialog.repaint();
				}
			});

			try {
				runKleenet(commandOutput);
			} catch (Exception e) {
				/* KleeNet was terminated - did it produce any results? */
				testInfo = new File(workingDir, kleeOutputDirectory + "/info");
				if (!testInfo.exists()) {
					testInfo = null;

					/* Kill tree graph generation */
					generateTreeThread.setName("stop");
					generateTreeThread.interrupt();

					/* Fail: no test output */
					throw e;
				}
			}

			/* Kill tree graph generation */
			generateTreeThread.setName("stop");
			generateTreeThread.interrupt();

			/* Dispose command dialog */
			disposeCommandOutputDialog();
		};
	};
	private JButton runButton = new JButton(runAction);

	private static class Concrete {
		int mote;
		int dscenario;
		boolean hasErr = false;
		String error;
		private int nextValue = 0;

		/* [ "varname", "var size as int", "string value as bytes" ] */
		ArrayList<JSONArray> accesses = new ArrayList<JSONArray>();

		public Concrete(int mote, int dscenario, JSONArray accesses, boolean hasErr, String error) {
			this.mote = mote;
			this.dscenario = dscenario;
			this.hasErr = hasErr;
			this.error = error;

			/* adding concrete values */
			for (int i=0; i < accesses.size(); i++) {
				/* TODO Special variable: sym */
				JSONArray access = (JSONArray)accesses.get(i);
				if (getAccessVarname(access).compareTo("sym") == 0) {
					/*logger.info("Ignoring special symbolic variable: " + getAccessVarname(access));*/
					continue;
				}

				this.accesses.add((JSONArray)accesses.get(i));
				/*logger.info("Added access varname: " + getAccessVarname((JSONArray)accesses.get(i)));*/ 
			}

			/*logger.debug("Added concrete: " + mote + "," + dscenario + "," + hasErr + "," + error + "," + accesses.size());*/
		}

		public int alreadyUsedValues() {
			return nextValue;
		}
		public boolean nextVarnameIs(String varname) {
			if (nextValue >= accesses.size()) {
				/*logger.debug("No more values");*/
				return false;
			}
			JSONArray access = accesses.get(nextValue);
			/*logger.debug("Comparing: " + getAccessVarname(access) + " vs " + varname);*/
			return getAccessVarname(access).equals(varname);
		}
		public byte[] getNextValue(String varname) {
			/* TODO return byte array */
			if (!nextVarnameIs(varname)) {
				logger.debug("Bad concrete request: " + varname + "@" + mote + "[" + nextValue + "]");
				return null;
			}
			JSONArray access = accesses.get(nextValue);
			byte[] val = getAccessValue(access);
			nextValue++; /* increments when value is returned */
			return val;
		}

		private static String getAccessVarname(JSONArray access) {
			return (String) access.get(0);
		}
		private static int getAccessSize(JSONArray access) {
			return ((Long) access.get(1)).intValue();
		}
		private static byte[] getAccessValue(JSONArray access) {
			byte[] mem = new byte[getAccessSize(access)];
			String byteStr = (String)access.get(2);
			assert(mem.length == byteStr.length());
			for (int i=0; i < byteStr.length(); i++) {
				mem[i] = (byte)byteStr.charAt(i);
			}
			return mem;
		}
	}
	private static Concrete getConcrete(ArrayList<Concrete> allConcrete, String varname, int moteID, int dscenario) {
		for (Concrete c: allConcrete) {
			if (c.mote != moteID) {
				continue;
			}
			if (c.dscenario != dscenario) {
				continue;
			}
			if (!c.nextVarnameIs(varname)) {
				continue;
			}

			return c;
		}

		return null;
	}
	private static int getTotalConcreteValuesCount(ArrayList<Concrete> allConcrete, int dscenario, boolean onlyUsed) {
		int count = 0;
		for (Concrete c: allConcrete) {
			if (c.dscenario != dscenario) {
				continue;
			}
			if (onlyUsed) {
				count += c.alreadyUsedValues();
			} else {
				count += c.accesses.size();
			}
		}
		return count;
	}
	private Workload simulateAction = new Workload("Parse & Simulate") {
		private static final long serialVersionUID = 7964400063143192035L;
		private TimeEvent monitorTimeEvent = null;
		private boolean stopMonitoring = false;

		public void work() throws Exception {
			/* Simulation state */
			if (sim.getSimulationTime() > 0) {
				throw new Exception("Simulation must have simulation time 0! Reload simulation.");
			}
			workingDir = sim.getMoteTypes()[0].getContikiSourceFile().getParentFile();
			if (kleeOutputDirectory == null) {
				/* Guess next KleeNet output directory */
				int counter = 0;
				while (true) {
					File outDir = new File(workingDir, "klee-out-" + counter);
					if (outDir.exists() && outDir.isDirectory()) {
						counter++;
					} else {
						outDir = new File(workingDir, "klee-out-" + (counter-1));
						kleeOutputDirectory = outDir.getName();
						break;
					}
				}
			}
			
			/* Setup dialog */
			setupCommandOutputDialog("Parsing & Simulating");

			/* Create readable ktest */
			try {

				/* Force recompilation of JSON export */
				/* XXX TODO should instead be phony target + fix .ktest deps */
				CompileContiki.compile(
						"rm -f " + kleeOutputDirectory + "/ktest.json.all",
						null,
						null,
						workingDir,
						null,
						null,
						commandOutput,
						true
				);

				/* Export .ktest to JSON format */
				CompileContiki.compile(
						"make TARGET=coojakleenet " + kleeOutputDirectory + "/ktest.json.all",
						null,
						null,
						workingDir,
						null,
						null,
						commandOutput,
						true
				);

				/* Force recompilation of concrete_queue module */
				CompileContiki.compile(
						"rm -f obj_cooja/concrete_queue.o",
						null,
						null,
						workingDir,
						null,
						null,
						commandOutput,
						true
				);

			} catch (Exception e) {
				MoteTypeCreationException e2 =
					new MoteTypeCreationException("Command failed: " + e.getMessage());
				e2 = (MoteTypeCreationException) e2.initCause(e);
				e2.setCompilationOutput(commandOutput);

				/* Print last 10 compilation errors to console */
				MessageContainer[] messages = commandOutput.getMessages();
				for (int i=messages.length-10; i < messages.length; i++) {
					if (i < 0) {
						continue;
					}
					logger.fatal(">> " + messages[i]);
				}

				logger.fatal("Command error: " + e.getMessage());
				throw e2;
			}

			/* Load and parse all ktests in JSON format */
			final ArrayList<Concrete> allConcrete = new ArrayList<Concrete>();
			String testCases = StringUtils.loadFromFile(new File(workingDir, kleeOutputDirectory + "/ktest.json.all"));
			try {
				Object obj = JSONValue.parse(testCases);
				JSONArray ktestArray = (JSONArray)obj;
				for (int i=0; i < ktestArray.size(); i++) {
					JSONObject ktestObj = (JSONObject)ktestArray.get(i);				
					allConcrete.add(new Concrete(
							((Long)ktestObj.get("node")).intValue(),
							((Long)ktestObj.get("dscenario")).intValue(),
							(JSONArray)ktestObj.get("objects"),
							(Boolean)ktestObj.get("hasErr"),
							(String)ktestObj.get("error")));
				}
			} catch (Exception pe) {
				throw (Exception) new Exception("JSON parse error:" + pe.getMessage()).initCause(pe);
			}

			/* Choose scenario to load */
			final int scenario = selectScenarioDialog(allConcrete);
			logger.info("Loading scenario: " + scenario);

			/* Create symbolic access monitor */
			dialog.addWindowListener(new WindowAdapter() {
				public void windowClosed(WindowEvent e) {
					if (monitorTimeEvent != null) {
						/* Disable monitor */
						stopMonitoring = true;
						monitorTimeEvent = null;

						for (Mote m: sim.getMotes()) {
							ContikiMote cm = (ContikiMote) m;
							SectionMoteMemory mem = (SectionMoteMemory) cm.getMemory();
							mem.setIntValueOf("concrete_queue_monitor_active", 0);
						}
					}
				}
			});
			
			monitorTimeEvent = new TimeEvent(0, "symbolic access monitor") {
				public void execute(long t) {
					/* -- Executed by simulation thread each millisecond -- */
					if (stopMonitoring) {
						return;
					}

					/* Did any mote request a concrete variable. */
					for (Mote m: sim.getMotes()) {
						ContikiMote cm = (ContikiMote) m;
						SectionMoteMemory mem = (SectionMoteMemory) cm.getMemory();

						if (mem.getIntValueOf("concrete_queue_error") == 1) {
							/* Bug was repeated */
							commandOutput.addMessage("### BUG WAS REPEATED! Stopping simulation");
							sim.stopSimulation(false);
						}

						while (mem.getIntValueOf("concrete_queue_requesting") == 1) {

							byte[] varnameArr = mem.getByteArray("concrete_queue_lastvar", 64);
							int destMote = mem.getIntValueOf("concrete_queue_dest_mote");
							String varname = new String(varnameArr).trim();

							if (destMote == 0) {
								/* Mote requested concrete variable. Set it, and execute immediately! */
								Concrete c = getConcrete(
										allConcrete, 
										varname, 
										cm.getID(), 
										scenario
								);
								if (c == null) {
									logger.fatal("ERROR: Bad symbolic access: " + varname + "/" + cm);
									commandOutput.addMessage("### ERROR: Bad symbolic access: " + varname + "/" + cm);
									mem.setIntValueOf("concrete_queue_lastvalue", -1);
								} else {
									/* TODO Check that the value is non-null when we return Java object... */
									byte[] value = c.getNextValue(varname);
									if (value != null) {
										commandOutput.addMessage("### Symbolic access " + 
												getTotalConcreteValuesCount(allConcrete, scenario, true) + "/" + 
												getTotalConcreteValuesCount(allConcrete, scenario, false) +
												": " + varname + "/" + cm);
										mem.setByteArray("concrete_queue_lastvalue", value);
									} else {
										mem.setIntValueOf("concrete_queue_lastvalue", -1);
									}
								}
							} else { /* Mote requested symbol from other mote */
								for (Mote m2: sim.getMotes()) {
									ContikiMote cm2 = (ContikiMote) m2;
									if (cm2.getInterfaces().getMoteID().getMoteID() == destMote) {
										varname = varname.substring(varname.indexOf("_") + 1);
										SectionMoteMemory mem2 = (SectionMoteMemory) cm2.getMemory();
										int destSymbolVal = mem2.getIntValueOf(varname);
										mem.setIntValueOf("concrete_queue_lastvalue", destSymbolVal);
										break;
									}
								}
							}

							mem.setIntValueOf("concrete_queue_requesting", 0);
							mem.setIntValueOf("concrete_queue_dest_mote", 0);

							try { Thread.sleep(200); } catch (InterruptedException e) { }

							/* Execute node immediately to avoid time drift */
							cm.execute(t);
						}
					}

					if (stopMonitoring) {
						return;
					}

					/* Reschedule next millisecond */
					sim.scheduleEvent(
							this, 
							sim.getSimulationTime() + Simulation.MILLISECOND);
				}
			};
			sim.scheduleEvent(
					monitorTimeEvent, 
					0 + 999 /* offset: 999 microseconds */);
			for (Mote m: sim.getMotes()) {
				ContikiMote cm = (ContikiMote) m;
				SectionMoteMemory mem = (SectionMoteMemory) cm.getMemory();
				mem.setIntValueOf("concrete_queue_monitor_active", 1);
			}
			commandOutput.addMessage("### Symbolic access monitor activated");

			commandOutput.addMessage("###");
			commandOutput.addMessage("###");
			commandOutput.addMessage("###    Repeat test case by running simulation now");
			commandOutput.addMessage("###              KEEP THIS DIALOG OPEN!");
			commandOutput.addMessage("###");
			commandOutput.addMessage("###");
			commandOutput.addMessage("### Total symbolic variable accesses in scenario: " + 
					getTotalConcreteValuesCount(allConcrete, scenario, false));
			dialog.setTitle(dialog.getTitle() + " (MONITORING SYMBOLIC ACCESS)");
		};
	};
	private JButton simulateButton = new JButton(simulateAction);

	private int selectScenarioDialog(ArrayList<Concrete> concretes) {

		final HashMap<Integer, Concrete> dscenarios = new HashMap<Integer, Concrete>();
		for (Concrete c: concretes) {
			if (!dscenarios.containsKey(new Integer(c.dscenario))) {
				dscenarios.put(new Integer(c.dscenario), c);
			} else if (c.hasErr) {
				/* there must be only one erroneous state in a dscenario */
				assert(!(dscenarios.get(new Integer(c.dscenario))).hasErr);
				dscenarios.put(new Integer(c.dscenario), c);
			} 
		}
		/* Errors on top */
		final ArrayList<Concrete> errors = new ArrayList<Concrete>();
		final ArrayList<Concrete> noErrors = new ArrayList<Concrete>();
		for (Concrete c: dscenarios.values()) {
			if (c.hasErr) {
				errors.add(c);
			} else {
				noErrors.add(c);
			}
		}

		/* Summary XXX TODO */
		String summaryText = StringUtils.loadFromFile(new File(workingDir, kleeOutputDirectory + "/info"));
		JTextArea summary = new JTextArea(summaryText);

		/* Scenarios */
		AbstractTableModel tableModel = new AbstractTableModel() {
			private static final long serialVersionUID = 6399790365414325505L;
			public String getColumnName(int col) {
				if (col == 0) {
					return "Scenario";
				} else {
					return "Error message";
				}
			}
			public int getColumnCount() {
				return 2;
			}
			public int getRowCount() {
				return dscenarios.size();
			}
			public Object getValueAt(int row, int col) {
				if (row < errors.size()) {
					if (col == 0) {
						return errors.get(row).dscenario;
					} else {
						return errors.get(row).error;
					}
				} else {
					row -= errors.size();
					if (col == 0) {
						return noErrors.get(row).dscenario;
					} else {
						/*return noErrors.get(row).error;*/
						return "";
					}
				}
			}
			public boolean isCellEditable(int row, int col){
				return false;
			}
			public Class<?> getColumnClass(int c) {
				return getValueAt(0, c).getClass();
			}
		};    
		JTable table = new JTable(tableModel);
		table.setSelectionMode(ListSelectionModel.SINGLE_SELECTION);
		table.setFillsViewportHeight(true);
		table.setFont(new Font("Monospaced", Font.PLAIN, 12));
		TableColumnAdjuster adjuster = new TableColumnAdjuster(table);
		adjuster.setDynamicAdjustment(true);
		adjuster.packColumns();

		final JDialog dialog = new JDialog((Frame) GUI.getTopParentContainer(), "Select scenario to repeat", true);

		/* Control */
		JButton button = new JButton("Load scenario");
		button.addActionListener(new ActionListener() {
			public void actionPerformed(ActionEvent e) {
				dialog.setVisible(false);
			}
		});

		/* Dialog */
		dialog.getContentPane().add(BorderLayout.NORTH, summary);
		dialog.getContentPane().add(BorderLayout.CENTER, new JScrollPane(table));
		dialog.getContentPane().add(BorderLayout.SOUTH, button);
		dialog.setSize(500, 500);
		dialog.setLocationRelativeTo(GUI.getTopParentContainer());
		dialog.setVisible(true);

		Integer s = (Integer) table.getValueAt(table.getSelectedRow(), 0);
		return s.intValue();
	}

	public Collection<Element> getConfigXML() {
		ArrayList<Element> config = new ArrayList<Element>();

		for (Mote m: sim.getMotes()) {
			/* Packet drops */
			if (motePacketDrops.containsKey(m) && motePacketDrops.get(m) != 0) {
				Element element = new Element("packetdrops");
				element.setAttribute("mote", "" + m.getID());
				element.setText("" + motePacketDrops.get(m));
				config.add(element);
			}

	        /* Subseuqent packet drops */
            if (motePacketSubsequentDrops.containsKey(m) && motePacketSubsequentDrops.get(m) != 0) {
              Element element = new Element("subsequent_packetdrops");
              element.setAttribute("mote", "" + m.getID());
              element.setText("" + motePacketSubsequentDrops.get(m));
              config.add(element);
            }

	        /* Packet duplicates */
            if (motePacketDuplicates.containsKey(m) && motePacketDuplicates.get(m) != 0) {
              Element element = new Element("duplicates");
              element.setAttribute("mote", "" + m.getID());
              element.setText("" + motePacketDuplicates.get(m));
              config.add(element);
            }
			
			/* Reboots */
			if (moteReboots.containsKey(m) && moteReboots.get(m) != 0) {
				Element element = new Element("reboots");
				element.setAttribute("mote", "" + m.getID());
				element.setText("" + moteReboots.get(m));
				config.add(element);
			}

			/* Outages */
			if (moteOutages.containsKey(m) && moteOutages.get(m) != 0) {
				Element element = new Element("outages");
				element.setAttribute("mote", "" + m.getID());
				element.setText("" + moteOutages.get(m));
				config.add(element);
			}

			/* Failure delay */
			if (moteFailureDelays.containsKey(m) && moteFailureDelays.get(m) != 0) {
				Element element = new Element("failure_delay");
				element.setAttribute("mote", "" + m.getID());
				element.setText("" + moteFailureDelays.get(m));
				config.add(element);
			}
		}
		return config;
	}

	public boolean setConfigXML(Collection<Element> configXML, boolean visAvailable) {
		for (Element element : configXML) {
			if (element.getName().equals("packetdrops")) {
				int id = Integer.parseInt(element.getAttributeValue("mote"));
				Mote m = sim.getMoteWithID(id);
				motePacketDrops.put(m, Integer.parseInt(element.getText()));
			} else if (element.getName().equals("subsequent_packetdrops")) {
			  int id = Integer.parseInt(element.getAttributeValue("mote"));
                Mote m = sim.getMoteWithID(id);
                motePacketSubsequentDrops.put(m, Integer.parseInt(element.getText()));
			} else if (element.getName().equals("duplicates")) {
			  int id = Integer.parseInt(element.getAttributeValue("mote"));
                Mote m = sim.getMoteWithID(id);
                motePacketDuplicates.put(m, Integer.parseInt(element.getText()));
			} else if (element.getName().equals("reboots")) {
				int id = Integer.parseInt(element.getAttributeValue("mote"));
				Mote m = sim.getMoteWithID(id);
				moteReboots.put(m, Integer.parseInt(element.getText()));
			} else if (element.getName().equals("outages")) {
				int id = Integer.parseInt(element.getAttributeValue("mote"));
				Mote m = sim.getMoteWithID(id);
				moteOutages.put(m, Integer.parseInt(element.getText()));
			} else if (element.getName().equals("failure_delay")) {
				int id = Integer.parseInt(element.getAttributeValue("mote"));
				Mote m = sim.getMoteWithID(id);
				moteFailureDelays.put(m, Integer.parseInt(element.getText()));
			}
		}
		return true;
	}
}
