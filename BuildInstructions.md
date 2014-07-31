# Detailed build steps:

### *Incomplete!*

### *This tutorial currently assumes that you're using a breadboard instead of the shield and not worrying about the enclosure!*

### Wiring is setup for the HAS_SHIELD layout, primarily for forward compatibility.


0. Acquire materials and build enclosure
	1. Make sure you have the relevant parts:
		* WildFire V3.1 with RFM69 removed/disabled
		* Red raygun button
		* Simple switch (for memory reset switch)
		* Potentiometer (~10K Ohms)
		* 16x2 LCD display
		* 14 male to female wires (preferably with some red, black/grey, yellow, and orange wires)
		* 2-11 male-male wires (non-breadboard version should have only those used for the button & switch)
		* 4x1 and 16x1 rows of male pins

		* Hopefully parts for the barrel jack & soldered on jumper.

	2. Make barreljack monstrosity, possibly power switch
		* Some steps
		* Solder on a wire to the power side of the barrel jack cable. This will power the K-30 sensor.
		* more steps

	3. Assemble the enclosure
		* A bunch of steps

1. Program and configure WildFire
	1. Load CUCO2_batched onto a WildFire
	2. Follow the steps for configuring the sketch in the github repository.

2. Setup breadboard
	The final breadboard will look something like this:
	* 2 wires connected to 11.1 volts
	* 3 wires connected to 5 volts
	* 7 wires connected to ground (8 with my battery powered kludge)
	* Possibly 2 wires connected to the potentiometer's output.
	(I'm counting both the input and output wires here - e.g. the wires connected to 5 volts are: potentiometer power, LCD power, and the WildFire's 5 volt output)

	Connect the WildFire's 5V output and ground to the breadboard. It doesn't really matter which ground terminal(s) you use on the WildFire.

	* If your potentiometer gets pushed directly into the breadboard, make sure the 5 volt row, potentiometer's output, and ground are positioned so the potentiometer can straddle all 3.

	* My breadboard needed it's ground connections to be on two different rows and a jumper to cross them.

3. Solder header pins onto LCD
	This one is easy(ish):
	Solder a row of 16 male headers pins to the LCD.
	Make sure that the long part of the pin comes out the back of the LCD display so that it can fit flush into its hole in the enclosure.
	We only care about the ones labeled in the diagram below, but it is easy enough to solder all of them.

	```
	---------------------------------------------------------------------------
	|                                                                         |
	|                                                                         |
	|                                                                         |
	|                                                                         |
	---------------------------------------------------------------------------
	D7 D6 D5 D4 . . . . E R/W RS V0 Vcc Vss . .
	(facing the screen)
	```

4. Solder header pins onto K-30 sensor
	Solder 4 male header pins to the back of the K-30 sensor.
	These should correspond to TX, RX, Vcc, and gnd in the diagram below.
	The pins should be sticking out of the back, so that the sensor part of the board can still poke out of its hole in the enclosure.

	```
	------------------------>
	| O        /\   _      (
	 ==>       \ './ \      |
	| ..     ,-'      |     |
	| ..   ,'         ;   O |
	| ..  /          ;   -- |
	 )   |          ;    -- |
	|[XX] \,___,,.-'     -- |
	|[XX]                <==
	|:::::::::::::::::::  O |
	-------------------------

	Pins by the bottom of this diagram (left to right, front view):

	14 holes, TX, RX, Vcc, gnd, 1 hole
	. . . . . 20 empty holes . . . . .
	```

5. Attaching K-30 to WildFire (v3.1 with RFM69 removed)
	1. Attach jumper wires to K-30.
		I've been using a color scheme for the (female-male) jumpers:
		(corner to center order)

		Connection | Wire color
		---------- | ----------
		Ground     | Black/Grey
		Vcc        | Red
		Transmit   | Orange
		Receive    | Yellow

	2. Plug Tx & Rx into WildFire

		K-30 | WildFire
		---- | --------
		Tx   | d2		(yellow)
		Rx   | d3		(orange)

	3. Plug breadboard into K-30

		K-30 | Breadboard
		---- | --------
		Vcc  | A row that will be connected to 11.1V power
		gnd  | A ground row (battery ground and WildFire ground will be connected)

6. Attach button, memory switch, and potentiometer to assembly
	1. Attach memory clearing switch, red button, and potentiometer to ground (on breadboard)
		How the potentiometer attaches varies based on what type of potentiometer you have.
	2. Attach potentiometer to 5v on breadboard

	3. Attach button and switch to WildFire

		Button/switch | WildFire
		------------- | --------
		Memory switch | a1
		Button        | a0

7. Attaching LCD display to assembly
	```
	---------------------------------------------------------------------------
	|                                                                         |
	|                                                                         |
	|                                                                         |
	|                                                                         |
	---------------------------------------------------------------------------
	D7 D6 D5 D4 . . . . E R/W RS V0 Vcc Vss . .
	(facing the screen)
	```
	1. Attach wires to the relevant LCD terminals.
		Connect (female-male) wires to each of the labeled headers in the diagram.
		No particular color scheme.

		The 4 pins closest to the corner get wired, next 4 don't, next 6 do, last 2 don't have wires.
	2. Connect LCD D4-D7 to WildFire
		Attach the LCD's pins to the WildFire like so:

		LCD  | WildFire
		---- | --------
		D4   | d4
		D5   | d5
		D6   | d6 		(WildFire d7 is skipped)
		D7   | d8		(corner of LCD)

	3. Connect remaining LCD pins to WildFire and breadboard

		LCD  | WildFire/breadboard
		---- | -------------------
		E    | a2
		R/W  | Breadboard ground
		RS   | a3
		V0   | Attach to potentiometer output
		Vcc  | Breadboard 5V power row
		Vss  | Breadboard ground

	Congratulations, now you have a rat's nest of wires. The clutter should get a lot better when we actually get the shields.

8. Connect power
	I've currently got a terrible kludge for my power setup:
		I'm missing the barreljack thing & the power switch, so I've connected the battery to the breadboard and plugged the WildFire into my computer.

		WildFire/breadboard  | Power sources
		-------------------- | -------------
		K-30 Vcc (via board) | Battery power
		Breadboard ground    | Battery ground
		USB-programmer       | Computer USB

9. Test
	1. Test LCD
		* If you've done everything correctly, the LCD display should light up and will begin its `Push to Smart Config` countdown).
		* If it is blank, adjust the potentiometer. If that doesn't fix it, you've probably got a problem with one of the 6 wires toward the middle of the LCD.
		* If the LCD is displaying gibberish, one of the 4 data (corner) wires is probably swapped, or you've got a flaky connection. Restart the WildFire after every change you make.

	2. Test CO2 sensor connection
		The K-30 should be occasionally blinking - this means it is properly powered and grounded.
		* Engage Offline mode to test readings
			1. Push button during `reconnecting` phase
			2. Wait until data is being recorded, if necessary clear old data & repeat.
			3. Make sure the readings given in offline mode are sensible (~400ppm is a normal CO2 concentration outside, inside it tends to be higher).
				* If it is giving 'bad reading', this either means that the power or ground for the K-30 isn't properly connected, or something more sinister is happening.
				* If it is constantly giving 2050 ppm, the power is disconnected.

10. Put everything in enclosure
	* Tape or glue things in?

11. Test!
	Test everything again, just to make sure you haven't jarred anything when putting it into the enclosure.
	You should also make sure the WildFire can connect to Wifi if you haven't already.