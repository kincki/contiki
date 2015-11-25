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
      TwistMoteType
      <identifier>apptype1</identifier>
      <description>Twist Mote Type #apptype1</description>
    </motetype>
    <mote>
      <interface_config>
        se.sics.cooja.motes.AbstractApplicationMoteType$SimpleMoteID
        <id>2</id>
      </interface_config>
      <interface_config>
        se.sics.cooja.interfaces.Position
        <x>52.52483344921173</x>
        <y>78.91307128431639</y>
        <z>0.0</z>
      </interface_config>
      <motetype_identifier>apptype1</motetype_identifier>
    </mote>
  </simulation>
  <plugin>
    se.sics.cooja.plugins.SimControl
    <width>259</width>
    <z>3</z>
    <height>205</height>
    <location_x>0</location_x>
    <location_y>0</location_y>
  </plugin>
  <plugin>
    se.sics.cooja.plugins.Visualizer
    <plugin_config>
      <skin>se.sics.cooja.plugins.skins.IDVisualizerSkin</skin>
      <viewport>5.958355957387749 0.0 0.0 5.958355957387749 -160.96165429290997 -349.19216840267063</viewport>
    </plugin_config>
    <width>300</width>
    <z>1</z>
    <height>300</height>
    <location_x>708</location_x>
    <location_y>0</location_y>
  </plugin>
  <plugin>
    se.sics.cooja.plugins.LogListener
    <plugin_config>
      <filter />
    </plugin_config>
    <width>1008</width>
    <z>5</z>
    <height>150</height>
    <location_x>0</location_x>
    <location_y>367</location_y>
  </plugin>
  <plugin>
    se.sics.cooja.plugins.TimeLine
    <plugin_config>
      <mote>0</mote>
      <showRadioRXTX />
      <showRadioHW />
      <showLEDs />
      <split>109</split>
      <zoomfactor>500.0</zoomfactor>
    </plugin_config>
    <width>1008</width>
    <z>4</z>
    <height>150</height>
    <location_x>0</location_x>
    <location_y>517</location_y>
  </plugin>
  <plugin>
    TwistPlugin
    <plugin_config>
      <server>localhost</server>
      <username>twistextern</username>
      <webusername>fros4943</webusername>
    </plugin_config>
    <width>436</width>
    <z>2</z>
    <height>424</height>
    <location_x>266</location_x>
    <location_y>9</location_y>
  </plugin>
  <plugin>
    se.sics.cooja.plugins.MoteInterfaceViewer
    <mote_arg>0</mote_arg>
    <plugin_config>
      <interface>Twist TinyOS Serial Forwarder connection</interface>
      <scrollpos>0,0</scrollpos>
    </plugin_config>
    <width>606</width>
    <z>0</z>
    <height>310</height>
    <location_x>341</location_x>
    <location_y>232</location_y>
  </plugin>
</simconf>

