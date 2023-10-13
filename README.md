# AOG_Section_control

<b>Section Control running on ESP32 for AgOpenGPS PC Software with GormR PCB</b><br><br>
Multi version code: supports V 4.3.10 (=version code 17) and V5.x.20 (=version code 20) including V5.7.*<br>
          
This sketch is forked from MTZ8302 and adapted to GormR's Section and Rate Control Unit PCB.<br>

Link to MTZ8302: https://github.com/mtz8302/AOG_SectionControl_ESP32/tree/2023-5.7.2-AgIO-heartbeat <br>
Link to GormR: https://github.com/GormR/HW_for_AgOpenGPS/tree/main/section_and_rate_control_unit
<br><br>
          
Supports: <br>- Section control, 8 sections (limited by PCB)
          <br>- hardware input switches (Main ON/OFF, OFF/Auto/ON for each section, +/- pressure (only motor driven, no rate control))
          <br>- documentation in AOG only at Auto
<br><br>
TODO: At the moment the sketch can't read back the status of the section switches. So the sketch has to be optimized to read back if a switch is set to OFF (red) or (manual) ON (yellow). <br>
Nevertheless it is possible to use the section switches physically to control the sprayer's valves.

Possible scenarios:
1) AUTO: if AOG sends "ON" then section will spray; if AOG sends "OFF" then section won't spray
2) OFF: AOG can send both states but the section will always be off and won't spray
3) ON: Section will spray whatever AOG wants to do	  


<b>!!!The ESP32 use 3.3V for any in-/outputs !!!</b>
Remark on non booting ESP32: they don't like raising input voltage when powering your step down 3,3V power source. To keep them in reset, put a 10uF electrolytic capacitor to reset - GND and another one EN - GND. So the enable and the reset pin will be LOW until power is stable. With this they will boot.

Suggestion for documentation only: https://agopengps.discourse.group/t/section-control-documentation-for-ferilizer-spreader/1360 <br>
Discourse thread for GormR PCB: https://discourse.agopengps.com/t/section-control-for-the-central-unit/7779