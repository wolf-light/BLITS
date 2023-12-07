# Instructions

## Assumptions
- You have a microcontroller that can connect to your computer over serial
- You have a general idea of what you're doing

## "Normal" Usage
### STIS UI
1. See the instructions.md file in the Pi folder
### Putty
- From Config File
  - Need to figure out how to do this on windows
  - Linux
    - Save config to ~/.putty/sessions
    - Load config or run by double clicking it
- Without config file
  - Ask someone if you need to save the data
  - Select Serial as connection type
  - Figure out the port name
  - Enter proper baudrate
  - Save config if you may need to connect multiple times
    - Enter sensible name in the dialog below "saved sessions"
    - double click to connect using the config
    - select config and then "load" to load and edit the config
  - Select open to open the connection


### Arduino IDE
1. Select port and board from context at top
2. `ctrl+shift+m` or magnifying glass in top right to open serial monitor

## Pushing Code
### Arduino IDE
1. Open sketch from file manager
2. Add board (for feather m0)
   1. Add third-party board manager: https://learn.adafruit.com/add-boards-arduino-v164/setup
3. Add libraries (need to start listing in top of .ino files actual name of library or including in lib folder, they're listed in Sketch/Include Library)
4. Connect board and select in dialog in top left

Notes:  
checkmark in top right verifies successful compilation  
right arrow in top right pushes the code to the board  
little button on board usually resets the microcontroller