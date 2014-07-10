CO<sub>2</sub> sensor firmware
==============================

Firmware for a WildFire & K-30 CO<sub>2</sub> sensor to interact with [this rails server](https://github.com/WickedDevice/CUCO2_Website)

CUCO2_batched is the current version of the WildFire sketch.

Organization of functions into the files is somewhat haphazard.

* `header.h` contains all of the parts of the configuration that is likely to change (at compile time).

* `CUCO2_batched.ino` - has loop, setup, and the configuration that is unlikely to change.

* `loop_functions.ino` - functions used in loop.

* `setup_functions.ino` - functions used in setup.

* `encryption.ino` - self contained encryption method using Vignere encryption.

* `memory_management.ino` contains all the functions related to memory. It should be entirely self-contained, & the implementation can be changed without modifying the rest of the program.


UI description
--------------

### Startup
When the WildFire turns on, it will display this text followed by a countdown:
```
Push button for
Smart Config 
```

* If it isn't displaying the countdown something has gone wrong.
	* If the LCD panel is showing `Invalid records...`, wait for it to finish clearing data, and then restart your device. It should behave normally after that.
	* If the LCD shows `No Encryption Key`, contact Wicked Device. If you are Wicked Device, plug the WildFire into a Serial Monitor & follow the instructions in the [configurationInstructions.md](./configurationInstructions.md) file.

### Connecting to Wifi (or choosing not to)
* If you want the WildFire to connect to a different network than it did last time, push the button.
	1. The LCD panel should display `Waiting for SmartConfig(60s)`. 
	Pull out your smartphone, startup Texas Instrument's CC3000 SmartConfig app, put in whatever details you need, and press "Start".
	2. If the connection is successful, the LCD will display `Connected` and `Requesting DHCP` after a few seconds.
* If there is no Wifi connection nearby:  
	1. Wait for the countdown to end
	2. Press and hold the button while the LCD display is showing `Reconnecting`
	3. The LCD should show `Offline Mode Engaged`. Wait for up to 10 seconds, and the sensor should begin recording.
* To reconnect to an old network, wait for the countdown to end.
	1. The LCD display will say `Reconnecting...`
		If this fails, then you probably need to push the  button in the previous step, restart your WildFire and try again.
	2. Then it will say `Reconnected. Requesting DHCP`

At this point, if the WildFire finds that it has old data, it will attempt to upload it. If you entered Offline mode, it will refuse to overwrite the data.


### Starting an experiment
Unless the WildFire is in Offline Mode, the LCD should display `Querying server    Hold to skip`.
* Holding the button down will cause the sensor to engage offline mode and begin collecting CO<sub>2</sub> readings.
* If the WildFire restarts after this step, your Wifi network may not be connected to the internet.
* If the LCD panel displays `No Experiment Found` before querying the server again, make sure you've setup and started an experiment on the website & selected this sensor.

Otherwise, the WildFire will pull down your information from the server & start the experiment.

### During an experiment
The WildFire should start recording data, and showing the ppm (parts per million).
On the second row of the LCD, it should display `Offline Mode` or `Hold to upload`.
* If it is displaying `Hold to upload`, holding the button down will pause the experiment and cause the WildFire to upload data. It will then check the server to see if the experiment is still active, and, if it is, will resume recording data.
* If it is displaying `Offline Mode`, holding the button will stop recording data.

The WildFire should stop recording data on its own if the threshold has been reached. (For Offline Mode, the default value is 2000ppm) It will also upload data if the WildFire runs out of memory. If it has done this, it will begin recording after querying the server again to see if the experiment parameters have changed or the experiment has ended (Offline mode will simply refuse to record more data).

* If the LCD displays `Bad reading`, then the sensor has returned a bad reading, and should fix itself in a few seconds and go back to recording data normally.
* If the LCD displays `Check sensor connection`, then the sensor isn't connected to the WildFire properly. This can also be caused when the sensor isn't getting power. The sensor should be blinking about once every two seconds if it is powered up.

### Uploading
Once the WildFire has finished collecting data, it will attempt to upload it.

If you're in offline mode, you'll need to restart the device to upload.

The LCD panel will display `Sending data...` until all data has been sent or there is an error.

If it displays `No connection`, the WildFire was unable to connect to the server. It will attempt to retry.
If it only displays `Upload failed    Retrying` it will have failed for some other reason, and attempt to send the data again.

### Complete
When uploading has finished, the LCD will show `Upload complete`. The WildFire will clear all the data read, and query the server to see if there is a new experiment waiting for it.

* If the WildFire has stopped recording because you pushed the button, after talking to the server it will attempt to record data for the same experiment it just uploaded for.

### Reseting memory
To clear all recorded data, flip the memory reset switch and push down the button during any of the following steps:
* While the `Push button for Smart Config` message is displayed
* While data is being recorded
* When offline mode has just begun, and the LCD displays `Old Data found   Aborted`
* In offline mode, after recording has ended and the LCD is displaying `Restart sensor  to upload`
The WildFire should just restart itself a second after clearing memory.


### Troubleshooting

If you're running into persistent errors, more detailed information is given through the serial port. Open up a serial monitor (eg. the one included with the Arduino IDE) on your computer and make sure to set your baud rate to 115200.

The WildFire has trouble connecting to the internet if the battery is low, or (on WildFire V2s) if the power is coming from the USB jack.


Hardware
--------
* WickedDevice WildFire (V3)
* K-30 CO<sub>2</sub>sensor
* 16x2 LCD display
* 10K potentiometer
* Battery and charger for WildFire
* Mini-breadboard (won't be in final product, will probably be replaced by a shield)

Wiring
------

In `header.h` there are two pin layouts defined: One for when the new Air Quality Egg shield is attached, and one without it. Which of these is selected is determined by whether `WITH_SHIELD` is `true` or `false`.

If you're using a WildFire V3, you'll need to remove the RFM69 or R16 (the resitor), because the RFM69 uses the same pin as Serial1's receive line. This is because pin D2 doubles as an interrupt pin and is used by the RFM69.

Here are the pin connections, with and without the shield attached:
(Pins are counter-clockwise starting from the barrel jack on the WildFire)

Pin | Connection with shield | Connection without shield
--- | ---------------------- | -------------------------
ior |                        |     
rst |                        |     
3v3 |                        |     
5v  | LCD Vcc, potentiometer power | LCD Vcc, potentiometer power
gnd | [\*](#ground-note)     | [\*](#ground-note)
gnd | [\*](#ground-note)     | [\*](#ground-note)
vin |                        |     
----| ----                   | ----
A0  | Button                 |     
A1  | Memory clearing switch | LCD RS
A2  | LCD Enable             | LCD Enable
A3  | LCD RS                 | LCD D4
A4  |                        | LCD D5
A5  |                        | LCD D6
A6  |                        | LCD D7
A7  |                        |     
----| ----                   | ----
RX  |                        |     
TX  |                        |     
D2  | K-30 TX                | K-30 TX
D3  | K-30 RX                | K-30 RX
D4  | LCD D4                 |     
D5  | LCD D5                 | Button
D6  | LCD D6                 | Memory clearing switch
D7  |                        |     
----| ----                   | ----
D8  | LCD D7                 |     
D9  |                        |     
D10 |                        |     
D11 |                        |     
D12 |                        |     
D13 |                        |     
gnd | [\*](#ground-note)     | [\*](#ground-note)
aref|                        |     
sda |                        |     
scl |                        |     

<a name="ground-note">\*</a> Ground is connected to: LCD Vss & R/W, potentiometer ground, K-30 ground, button ground, and Memory clearing switch ground

The LCD, from left to right (facing the screen, pins on the bottom left):
```
---------------------------------------------------------------------------
|																		  |
|																		  |
|																		  |
|																		  |
---------------------------------------------------------------------------
D7 D6 D5 D4 . . . . E R/W RS V0 Vcc Vss . .
```

V0 is connected to the 10K potentiometer.


The sensor:
```
------------------------>
| O        /\   _ 	   (
 ==>       \ './ \	    |
| ..     ,-'      |	    |
| ..   ,'         ;	  O |
| ..  /          ;   -- |
 )   |          ;    -- |
|[XX] \,___,,.-'     -- |
|[XX]                <==
|:::::::::::::::::::  O |
-------------------------
Pins by the bottom of this diagram:

14 holes, TX, RX, Vcc, gnd, 1 hole
. . . . . 20 empty holes . . . . .
```
