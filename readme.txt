scp -i ~/.ssh/Matus.key index.html radio.py  matus@192.168.2.18:~/python/
ssh matus@192.168.2.18 -i ~/.ssh/Matus.key

scp -i ~/.ssh/Matus.key -r /home/dafe/git/WaterLevelInTank/Rapsberry/ha-mqtt matus@192.168.2.18:~/python/




https://howtomechatronics.com/tutorials/arduino/arduino-and-hc-12-long-range-wireless-communication-module/#google_vignette
https://projecthub.arduino.cc/Manusha_Ramanayake/wireless-water-tank-level-meter-with-alarm-ce92f6

https://www.pi4j.com/1.2/pins/model-b-rev1.html
https://docs.arduino.cc/resources/pinouts/A000005-full-pinout.pdf

https://tutorials.probots.co.in/communicating-with-a-waterproof-ultrasonic-sensor-aj-sr04m-jsn-sr04t/

--- 
power & sleep

https://docs.arduino.cc/learn/electronics/low-power/

https://github.com/RalphBacon/Arduino-Deep-Sleep/blob/master/Sleep_ATMEGA328P_Timer.ino

https://forum.arduino.cc/t/best-approach-to-make-arduino-sleep-then-wake-up-and-take-a-measurement/318632/3

https://gist.github.com/stojg/aec2c8c54c29c0fab407

https://www.youtube.com/watch?v=usKaGRzwIMI

https://www.youtube.com/watch?v=5cYN5-Spnos&t=240s

----
AtTiny 
https://www.instructables.com/ATTinyCore-HW-260-and-the-ATTINY85/
fuse calculator

USBISP
https://wiki.fryktoria.com/doku.php?id=arduino:how-to-fix-a-usb-isp-programmer-and-make-it-work-with-arduino-ide-on-linux

---- 
Home asistant

Server installation via kvm
https://www.home-assistant.io/installation/linux

http://VMIP:8123/

mqtt to home assistant

Mosquito broker addon 
MoSquito integration
https://www.home-assistant.io/integrations/mqtt
https://github.com/home-assistant/addons/blob/master/mosquitto/DOCS.md


--- 
Rapsberry 

https://github.com/unixorn/ha-mqtt-discoverable?tab=readme-ov-file#usage-8
sudo pip3 install paho-mqtt