# Tic_Tac_Toe_132_final

This was the final project of the Microcontroller Laboratory at Lehigh University. It is a tic tac toe game that uses UART to interact with the user. It was implemented using code
composer studio and ran on the Tiva Launchpad. We had to manipulate the LED to indicate whether a user won or lost, use UART  connection to display the board, and hibernate when
the user does not give an input for a while.

The program was implemented using no polling and only interrupts. Delays we not hardcoded, rather only timers were used for a delay.
