# Bird-Bath-Automated-Controller
This project was made to upgrade an existing bird bath controller which cleaned/filled a bird bath periodically. The new controller has an LCD interface, increased reliability, and more control options.

For the LCD interface, the program utilizes a finite-state machine in which each state corresponds to a menu option for the LCD. There are currently eight states, or screens, as stated:
h	Home screen that shows the last operation, the next operation, and the operation after the next.
e	Disable system screen. Asks whether to disable system.
n	Night mode screen. Asks whether to turn on/off night mode.
l	Schedule clean screen. Increment or decrement the number of cleans per fill.
i	Schedule fill screen. Determine how often a fill will occur.
f	Fill screen. Only shows when the system is filling.
c	Clean screen. Only shows when the system is cleaning.
d	Disabled screen. Shows when the system is disabled.

The system will store the letter of the screen in a char variable called “mode”. In the main loop, there is a switch statement that is determined by mode. The switch statement will show the corresponding LCD screen. For example, if mode is currently ‘l’, the LCD will show the schedule clean screen. To recognize a button press, the program uses interrupts. Currently the LCD has four buttons associated with it that are connected to pins 0-3 of port B of the ATmega4809. When a button is pressed, the program goes to the interrupt service routine for port B. Depending on what mode the program is currently on, the buttons will perform different actions. If the mode is currently ‘h’, the four buttons will act as the disable system button, the night mode button, the schedule clean button, and the schedule fill button, whereas if the mode is ‘e’, the buttons will either disable the system or go back to the home screen.
