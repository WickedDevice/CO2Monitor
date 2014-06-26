Configuring the CO2 sensor sketch:

#Encryption key & Mac Address Configuration:

* Load the CUCO2_batched sketch onto the WildFire

* Open up a serial monitor in the Arduino IDE

* Make sure the serial monitor's type is 'Newline' (bottom right corner of the window)

* Type something into the serial monitor just as the WildFire is starting
	(If the LCD isn't displaying 'Push button for Smart Config', it has been too long)

* Wait for the device's Mac Address to appear on the serial output

* When the serial gives you a y/n prompt, type in a y (or Y), then enter.
	(If an old encryption key is found, it will print that out)
	If you don't respond fast enough, the WildFire will restart (this is to prevent accidents once the WildFire is in the field)

* The WildFire will ask for an encryption key. Ignore it for now.

* Navigate to the CUCO2 website
	( http://107.170.187.156 )

* Login as an admin

* Navigate to the 'Create new sensor' form
	('Sensors' -> 'Create new sensor' link at bottom of table)

* Enter in appropriate information into the form
	Copy the Mac address from the serial monitor to the website

* Enter in an encryption key into both the web form and the serial monitor.
	(31 characters or less, any printable character is legal, no tabs or newlines)

* Check in serial monitor if information is correct, if so, type in 'Y', then enter.

* Submit web form

* The sensor should restart, properly configured.


#Smart Config wifi configuration:
This needs to be done every time the WildFire connects to a new network

* Press the button on the sensor right after it restarts.
	(The lcd display should say 'Push button for Smart Config')

* Open up TI's Smart Config App on your smartphone/tablet

* Make sure the WIFI network is correct, and enter in the password

* Press 'Start'

* The Sensor should get information from the phone, and connect to the wifi network.
