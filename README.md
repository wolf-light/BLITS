**BLITS (Ballistic Long-range Ignition Testing System) and STIS (Static Test Ignition System):**
  The BLITS system integrates with the Avionics STIS (Static Test Ignition System) to provide advanced pressure and temperature data acquisition capabilities provided by the Hybrid committee. The BLITS software is designed to operate on a Raspberry Pi. The STIS Boombox interfaces with the Raspberry Pi via a single USB cable, delivering thrust, pressure, and temperature data provided by the BLITS Sensor suite. The Raspberry Pi efficiently processes and stores incoming data.
  The BLITS System Sensor Suite not only oversees the collection of rocketry motor test data but also communicates with the Feather M0 which handles the long-range communication between the Boombox and the Football.
  This integrated system represents a fusion of the Avionics Committee's Static Test Ignition System and the Hybrid Committee's Hybrid Control System. Upon completion, it will offer compatibility for deployment in solid motor tests, hybrid motor tests, and the strand burner project.

**Lead Contributors:**
Ian Frantz - Avionics Lead
Jacob Korkowski - Hybrid Lead / Safety Lead
Dagan Larson - BLITS System Sensor Suite & RF Dev
Adam Chiarella - BLITS System Raspberry Pi Software Dev

**Definitions:**
**STIS: Static Test Ignition System**
	Consists of: Boombox, Football, BLITS System Sensor Suite
	Used for: Full-Scale solid motor tests, Data collection, motor ignition
	Notes: Current Avionics System for Full-Scale Motor Tests
**BLITS System: Ballistic Long-range Ignition Testing Synergy System**
	Consists of: Sensor Suite
	Used for: Universal Rocketry Ignition and Data Collection
	Notes: Section of the STIS dealing with data collection and motor ignition
**HCS: Hybrid Control System**
	Consists of: Arduino Nano, Misc Sensors
	Used for: Data collection, motor ignition
	Notes: Hybrid system used for 22-23 academic year, 23-24 small-scale tests
**Boombox: Assembly of our sensor and ignition components**
	Consists of: Feather M0 (RF Communication), BLITS System Sensor Suite
	Used for: Full-Scale test data collection and motor ignition
	Notes: Component of the STIS
**Football: Handheld housing/assembly allowing remote RF Communication/control of test**
	Consists of: Feather M0 (RF Communication)
	Used for: Full-Scale test control, fire control, data collection control
	Notes: Component of the STIS
