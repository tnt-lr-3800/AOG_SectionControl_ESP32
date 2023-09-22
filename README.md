# AOG_Section_control

<b>Section Control running on ESP32 for AgOpenGPS and GormR's Section and Rate Control PCB.</b><br><br>

          
Code is based on MTZ's branch "AOG_SectionControl_ESP32/2023-5.7.2-AgIO-heartbeat" which was adapted to AOG 5.7.*. <br>
PCB can be found here: https://github.com/GormR/HW_for_AgOpenGPS/tree/main/section_and_rate_control_unit

At the moment this sketch can be used in automatic mode (data via USB or WiFi). Switches can set relay to AOG-Automatic, Off and manual On.
It needs further development to read back switch status to ESP32 and AOG.
