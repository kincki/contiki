<?xml version="1.0" encoding="UTF-8"?>
<simconf>
  <project EXPORT="discard">[CONTIKI_DIR]/tools/cooja/apps/mrm</project>
  <project EXPORT="discard">[CONTIKI_DIR]/tools/cooja/apps/mspsim</project>
  <project EXPORT="discard">[CONTIKI_DIR]/tools/cooja/apps/avrora</project>
  <project EXPORT="discard">[CONTIKI_DIR]/tools/cooja/apps/serial_socket</project>
  <project EXPORT="discard">[CONTIKI_DIR]/tools/cooja/apps/collect-view</project>
  <project EXPORT="discard">C:/home/contiki-projects/sics.se/powertracker</project>
  <project EXPORT="discard">C:/home/contiki-projects/sics.se/mobility</project>
  <project EXPORT="discard">C:/home/fros-tmp/cooja-projects/quick-deploy</project>
  <project EXPORT="discard">C:/home/nes/papers/contention2-paper/code/strawman_radiologger</project>
  <project EXPORT="discard">C:/home/nes/papers/smartip-paper/code/cooja_qr</project>
  <project EXPORT="discard">C:/home/fros-tmp/robot-testbed/cooja-player</project>
  <project EXPORT="discard">C:/home/fros-tmp/robot-testbed/javaclient_download</project>
  <project EXPORT="discard">[CONTIKI_DIR]/tools/cooja/apps/native_gateway</project>
  <project EXPORT="discard">C:/home/fros-tmp/cooja-projects/carlo11ewsn</project>
  <project EXPORT="discard">C:/home/contiki-projects/sics.se/frossi-scanner</project>
  <project EXPORT="discard">[CONFIG_DIR]/..</project>
  <simulation>
    <title>My simulation</title>
    <delaytime>-2147483648</delaytime>
    <randomseed>123456</randomseed>
    <motedelay_us>1000000</motedelay_us>
    <radiomedium>
      se.sics.cooja.radiomediums.UDGM
      <transmitting_range>50.0</transmitting_range>
      <interference_range>100.0</interference_range>
      <success_ratio_tx>1.0</success_ratio_tx>
      <success_ratio_rx>1.0</success_ratio_rx>
    </radiomedium>
    <events>
      <logoutput>400000</logoutput>
    </events>
    <motetype>
      se.sics.cooja.mspmote.SkyMoteType
      <identifier>sky1</identifier>
      <description>Sky Mote Type #sky1</description>
      <source EXPORT="discard">C:/home/contiki_clean/examples/sky-shell/sky-shell.c</source>
      <commands EXPORT="discard">make -j 20 sky-shell.sky TARGET=sky</commands>
      <firmware EXPORT="copy">C:/home/contiki_clean/examples/sky-shell/sky-shell.sky</firmware>
      <moteinterface>se.sics.cooja.interfaces.Position</moteinterface>
      <moteinterface>se.sics.cooja.interfaces.RimeAddress</moteinterface>
      <moteinterface>se.sics.cooja.interfaces.IPAddress</moteinterface>
      <moteinterface>se.sics.cooja.interfaces.Mote2MoteRelations</moteinterface>
      <moteinterface>se.sics.cooja.interfaces.MoteAttributes</moteinterface>
      <moteinterface>se.sics.cooja.mspmote.interfaces.MspClock</moteinterface>
      <moteinterface>se.sics.cooja.mspmote.interfaces.MspMoteID</moteinterface>
      <moteinterface>se.sics.cooja.mspmote.interfaces.SkyButton</moteinterface>
      <moteinterface>se.sics.cooja.mspmote.interfaces.SkyFlash</moteinterface>
      <moteinterface>se.sics.cooja.mspmote.interfaces.SkyCoffeeFilesystem</moteinterface>
      <moteinterface>se.sics.cooja.mspmote.interfaces.SkyByteRadio</moteinterface>
      <moteinterface>se.sics.cooja.mspmote.interfaces.MspSerial</moteinterface>
      <moteinterface>se.sics.cooja.mspmote.interfaces.SkyLED</moteinterface>
      <moteinterface>se.sics.cooja.mspmote.interfaces.MspDebugOutput</moteinterface>
      <moteinterface>se.sics.cooja.mspmote.interfaces.SkyTemperature</moteinterface>
    </motetype>
    <mote>
      <breakpoints />
      <interface_config>
        se.sics.cooja.interfaces.Position
        <x>58.81219931544578</x>
        <y>99.15594186559494</y>
        <z>0.0</z>
      </interface_config>
      <interface_config>
        se.sics.cooja.mspmote.interfaces.MspMoteID
        <id>1</id>
      </interface_config>
      <motetype_identifier>sky1</motetype_identifier>
    </mote>
  </simulation>
  <plugin>
    se.sics.cooja.plugins.SimControl
    <width>259</width>
    <z>5</z>
    <height>205</height>
    <location_x>0</location_x>
    <location_y>0</location_y>
  </plugin>
  <plugin>
    se.sics.cooja.plugins.Visualizer
    <plugin_config>
      <skin>se.sics.cooja.plugins.skins.IDVisualizerSkin</skin>
      <viewport>0.9090909090909091 0.0 0.0 0.9090909090909091 90.53436425868566 29.858234667640964</viewport>
    </plugin_config>
    <width>291</width>
    <z>3</z>
    <height>293</height>
    <location_x>958</location_x>
    <location_y>10</location_y>
  </plugin>
  <plugin>
    se.sics.cooja.plugins.LogListener
    <plugin_config>
      <filter />
      <formatted_time />
      <coloring />
    </plugin_config>
    <width>641</width>
    <z>2</z>
    <height>297</height>
    <location_x>12</location_x>
    <location_y>312</location_y>
  </plugin>
  <plugin>
    se.sics.cooja.plugins.MoteInterfaceViewer
    <mote_arg>0</mote_arg>
    <plugin_config>
      <interface>Serial port</interface>
      <scrollpos>0,0</scrollpos>
    </plugin_config>
    <width>689</width>
    <z>1</z>
    <height>290</height>
    <location_x>262</location_x>
    <location_y>14</location_y>
  </plugin>
  <plugin>
    SerialSocketServer
    <mote_arg>0</mote_arg>
    <width>422</width>
    <z>4</z>
    <height>70</height>
    <location_x>798</location_x>
    <location_y>541</location_y>
  </plugin>
  <plugin>
    se.sics.cooja.plugins.LogListener
    <plugin_config>
      <filter>XXX</filter>
    </plugin_config>
    <width>569</width>
    <z>0</z>
    <height>214</height>
    <location_x>668</location_x>
    <location_y>309</location_y>
  </plugin>
</simconf>

