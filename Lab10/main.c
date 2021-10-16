// Final lab for ECE 132 - Lab 10: Tic-Tac-Toe Game
// Bishoy Youhana - 5/7/2021

//include l
#include <stdint.h>
#include <stdbool.h>
#include "inc/hw_ints.h"
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "driverlib/gpio.h"
#include "driverlib/interrupt.h"
#include "driverlib/pin_map.h"
#include "driverlib/sysctl.h"
#include "driverlib/uart.h"
#include "driverlib/hibernate.h"
#include "driverlib/timer.h"
#include <math.h>

//function declarations
int bestMove(char board[3][3]);
int minimax(char board[3][3], bool ai);
int min(int x, int y);
int max(int x, int y);
int winner(char board[][]);
bool checkInput(char input);
void indexMap();
void printMap();
int retrieveInput();
void blink(int color);

//global variables
int bestMoveY;
char board[3][3]; //board to be stored
bool LED; //monitor the state of the LED when needed

//messages
char intro[219] =
        "\n\rWelcome to tic tac toe! Please enter a number from 1 to 9, you are x and computer is o."
                "\n\rBoard will enter into hibernation if you do not enter a number in the next 5 seconds, to wake it up press SW2.\n\r";
char enterString[53] = "Enter a number from 1 to 9 that is not already chosen";
char enterValidString[39] = "Please enter a valid number from 1 to 9";
char xWon[46] = "\n\rX won !!!  Press reset to play again!!\n\r";
char yWon[46] = "\n\rO won !!!  Press reset to play again!!\n\r";
char draw[23] = "\n\rIt's a draw !!!\n\r";
char aiTurn[19] = "Computer turn: \n\r";
char hibernation[28] = "\n\rEntered Hibernation.\n\r";

//UART interrupt handler, is not called, is not used, and does not do anything
void UARTIntHandler(void)
{
    uint32_t ui32Status;
    ui32Status = UARTIntStatus(UART0_BASE, true); //get interrupt status
    UARTIntClear(UART0_BASE, ui32Status); //clear the asserted interrupts
}

//blinking timer interrupt handler, interrupt flips the state of the LED when executed
void Timer1IntHandler(void)
{
    TimerIntClear(TIMER1_BASE, TIMER_TIMA_TIMEOUT);

    if(LED){
        GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3,
                             0x00);
        LED = false;
    }else{
        GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_1, 14);
        LED = true;
    }
}

//hibernate timer interrupt handler, interrupt puts the board in hibernate mode when executed
void Timer0IntHandler(void)
{
    //TimerDisable(TIMER0_BASE, TIMER_A);
    int i;
    for (i = 0; i < 38; i++)
    {
        UARTCharPut(UART0_BASE, hibernation[i]);
    }

    TimerIntClear(TIMER0_BASE, TIMER_TIMA_TIMEOUT);

    HibernateGPIORetentionEnable();
    HibernateWakeSet(HIBERNATE_WAKE_PIN);
    HibernateRequest();
}

int main(void)
{
    SysCtlClockSet(
    SYSCTL_SYSDIV_5 | SYSCTL_USE_PLL | SYSCTL_OSC_MAIN | SYSCTL_XTAL_16MHZ); //set system clock to 40MHZ

    //UART
    SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0); //Enable the UART0
    IntEnable(INT_UART0); //enable the UART interrupt
    UARTConfigSetExpClk(
            UART0_BASE, SysCtlClockGet(), 115200,
            (UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE | UART_CONFIG_PAR_NONE));
    UARTIntEnable(UART0_BASE, UART_INT_RX | UART_INT_RT); //only enable RX and TX interrupts

    //GPIO
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA); //Enable GPIOA peripherals
    GPIOPinConfigure(GPIO_PA0_U0RX); //configure the pins for the receiver and transmitter using GPIOPinConfigure
    GPIOPinConfigure(GPIO_PA1_U0TX);
    GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);
    GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE,
    GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3); //initialize the GPIO peripheral and pin for the LED.

    //timer0
    SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER0);
    TimerConfigure(TIMER0_BASE, TIMER_CFG_PERIODIC);
    TimerLoadSet(TIMER0_BASE, TIMER_A, 200000000); //set to 5s
    IntEnable(INT_TIMER0A);
    TimerIntEnable(TIMER0_BASE, TIMER_TIMA_TIMEOUT);

    //timer1
    SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER1);
    TimerConfigure(TIMER1_BASE, TIMER_CFG_PERIODIC);
    TimerLoadSet(TIMER1_BASE, TIMER_A, 8000000); //set to 200ms
    IntEnable(INT_TIMER1A);
    TimerIntEnable(TIMER1_BASE, TIMER_TIMA_TIMEOUT);

    //hibernate
    SysCtlPeripheralEnable(SYSCTL_PERIPH_HIBERNATE);
    HibernateEnableExpClk(SysCtlClockGet());

    IntMasterEnable(); //enable processor interrupts

    int i = 0, j = 0;
    for (i = 0; i < 219; i++)
    {
        UARTCharPut(UART0_BASE, intro[i]);
    }

    //initialize board to 0
    for (i = 0; i < 3; i++)
    {
        for (j = 0; j < 3; j++)
        {
            board[i][j] = 0;
        }
    }

    // user first run
    TimerEnable(TIMER0_BASE, TIMER_A); // not wait more than 5 seconds
    indexMap();
    int x, y;

    int validInput = retrieveInput(board) - 48; //validInput stores value entered by user in integer for, input stores it in char form

    //sets x and y based on validInput
    y = validInput % 3 - 1;
    if (y == -1) y = 2;
    if (validInput < 10 && validInput > 5)  x = 2;
    if (validInput < 7 && validInput > 3)   x = 1;
    if (validInput < 4 && validInput > 0)   x = 0;

    board[x][y] = 'x'; //update board

    printMap(board);
    UARTCharPut(UART0_BASE, '\n');
    UARTCharPut(UART0_BASE, '\r');

    //subsequent runs
    while (winner(board) == 0)
    {
        //computer turn
        UARTCharPut(UART0_BASE, '\n');
        UARTCharPut(UART0_BASE, '\r');
        int bestMoveX;
        bestMoveX = bestMove(board); //get AI choice

        board[bestMoveX][bestMoveY] = 'o'; //update board
        for (i = 0; i < 19; i++)
        {
            UARTCharPut(UART0_BASE, aiTurn[i]);
        }
        printMap(board);

        //player turn
        indexMap();
        if (winner(board) != 0)
            break;
        for (i = 0; i < 53; i++)
        {
            UARTCharPut(UART0_BASE, enterString[i]);
        }
        UARTCharPut(UART0_BASE, '\n');
        UARTCharPut(UART0_BASE, '\r');

        // get input and set it to x and y
        int validInput = retrieveInput(board) - 48;

        y = validInput % 3 - 1;
        if (y == -1) y = 2;
        if (validInput < 10 && validInput > 5)  x = 2;
        if (validInput < 7 && validInput > 3)   x = 1;
        if (validInput < 4 && validInput > 0)   x = 0;

        board[x][y] = 'x'; //update board

        printMap(board);
        if (winner(board) != 0) break; //check if someone won

        UARTCharPut(UART0_BASE, '\n');
        UARTCharPut(UART0_BASE, '\r');
    }
    //someone won
    int checkWin = winner(board); //check who won, 1, xwon, 2, ywon, 3, draw

    //notify user
    switch (checkWin)
    {
    case 1:
        for (i = 0; i < 46; i++)
        {
            blink(3);
            UARTCharPut(UART0_BASE, xWon[i]);
        }
        break;
    case 2:
        for (i = 0; i < 46; i++)
        {
            blink(3);
            UARTCharPut(UART0_BASE, yWon[i]);
        }
        break;
    case 3:
        for (i = 0; i < 23; i++)
        {
            blink(2);
            UARTCharPut(UART0_BASE, draw[i]);
        }
        break;
    }
}

//function controls the LED. If color is 1, blink red LED twice, 2 is blue, and 3 is green.
void blink(int color)
{
    switch (color)
    {
    case 1:
        TimerEnable(TIMER1_BASE, TIMER_A);
        LED = true;
        GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_1, 14);
        while(LED){}
        TimerDisable(TIMER1_BASE, TIMER_A);

        TimerIntEnable(TIMER1_BASE, TIMER_TIMA_TIMEOUT);
        TimerLoadSet(TIMER1_BASE, TIMER_A, 4000000); // load 100 ms
        TimerEnable(TIMER1_BASE, TIMER_A);
        while (!LED){}
        TimerDisable(TIMER1_BASE, TIMER_A);

        TimerIntEnable(TIMER1_BASE, TIMER_TIMA_TIMEOUT);
        TimerLoadSet(TIMER1_BASE, TIMER_A, 8000000); // load 200 ms
        TimerEnable(TIMER1_BASE, TIMER_A);
        while (LED){}
        TimerDisable(TIMER1_BASE, TIMER_A);

        break;
    case 2:
        GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_2, 14);
        break;
    case 3:
        GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_3, 14);
        break;
    }
}

//function get proper input from user.
int retrieveInput()
{
    bool val = false;
    int validInput, input, i;
    while (!val) //loop while there are chars
    {
        if (UARTCharsAvail(UART0_BASE))
        {
            input = UARTCharGetNonBlocking(UART0_BASE);
            val = true;
            UARTCharPutNonBlocking(UART0_BASE, input);
            TimerDisable(TIMER0_BASE, TIMER_A); // there was an input disable timer
        }
        if (val)
        {
            if (checkInput(input))
            {
                validInput = input;
                val = true;
                UARTCharPut(UART0_BASE, '\n');
                UARTCharPut(UART0_BASE, '\r');
            }
            else
            {
                blink(1);
                UARTCharPut(UART0_BASE, '\n');
                UARTCharPut(UART0_BASE, '\r');
                for (i = 0; i < 39; i++)
                {
                    UARTCharPut(UART0_BASE, enterValidString[i]);
                }
                UARTCharPut(UART0_BASE, '\n');
                UARTCharPut(UART0_BASE, '\r');
                val = false;
            }
        }
    }
    return validInput;
}
//return true if input is from  1 to 9 and is not already chosen in the baord.
bool checkInput(char input)
{
    int newIn = input - 48;
    int x, y;
    if (newIn < 10 && newIn > 0)
    {
        x = newIn % 3 - 1;
        if (x == -1)
            x = 2;
        if (newIn < 10 && newIn > 5)
            y = 2;
        if (newIn < 7 && newIn > 3)
            y = 1;
        if (newIn < 4 && newIn > 0)
            y = 0;

        if (board[y][x] != 'x' && board[y][x] != 'o')
        {
            return true;
        }
        return false;
    }
    return false;
}

//used by bestscore function to get the minimax score
int minimax(char board[3][3], bool ai)
{
    //recursive score return
    int i = 0, j = 0;
    int checkWin = winner(board);
    if (checkWin != 0)
    {

        if (checkWin == 1)
        {
            return -10;
        }
        else if (checkWin == 2)
        {
            return 10;
        }
        else if (checkWin == 3)
        {
            return 0;
        }
    }
    int score = 0;
    int bestScore = 0;

    if (ai)
    {
        bestScore = -100000; //small random value since we want the maximum
        for (i = 0; i < 3; i++)
        {
            for (j = 0; j < 3; j++)
            {
                if (board[i][j] == 0)
                {
                    board[i][j] = 'o';
                    score = minimax(board, false);
                    board[i][j] = 0;
                    bestScore = max(score, bestScore);
                }
            }
        }
        return bestScore;
    }
    else
    {
        bestScore = 100000; //big random value since we want the minimum
        for (i = 0; i < 3; i++)
        {
            for (j = 0; j < 3; j++)
            {
                if (board[i][j] == 0)
                {
                    board[i][j] = 'x';
                    score = minimax(board, true);
                    board[i][j] = 0;
                    bestScore = min(score, bestScore);
                }
            }
        }
        return bestScore;
    }

    //return 1;
}

//function get the best probable move based on this algorithm, save the Y co-ordinate
//to the global variable bestMoveY and returns the x co-ordinate
int bestMove(char board[3][3])
{
    int i = 0, j = 0;
    int bestScore = -10000; //small random value since we want the maximum
    int currentScore;
    int bestMoveX;

    for (i = 0; i < 3; i++)
    {
        for (j = 0; j < 3; j++)
        {
            //spot available
            if (board[i][j] == 0)
            {
                board[i][j] = 'o';
                currentScore = minimax(board, false);
                board[i][j] = 0;
                if (currentScore > bestScore)
                {
                    bestScore = currentScore;
                    bestMoveX = i;
                    bestMoveY = j;
                }
            }
        }
    }
    return bestMoveX;
}

int min(int x, int y)   //get min
{
    if (x < y)
        return x;
    return y;
}

int max(int x, int y)   //get max
{
    if (x > y)
        return x;
    return y;
}

void printMap() //print current map
{
    UARTCharPut(UART0_BASE, '\n');
    UARTCharPut(UART0_BASE, board[0][0]);
    UARTCharPut(UART0_BASE, '\t');
    UARTCharPut(UART0_BASE, '|');
    UARTCharPut(UART0_BASE, '\t');
    UARTCharPut(UART0_BASE, board[0][1]);
    UARTCharPut(UART0_BASE, '\t');
    UARTCharPut(UART0_BASE, '|');
    UARTCharPut(UART0_BASE, '\t');
    UARTCharPut(UART0_BASE, board[0][2]);
    UARTCharPut(UART0_BASE, '\t');
    UARTCharPut(UART0_BASE, '|');
    UARTCharPut(UART0_BASE, '\t');
    UARTCharPut(UART0_BASE, '\n');
    UARTCharPut(UART0_BASE, '\r');
    UARTCharPut(UART0_BASE, board[1][0]);
    UARTCharPut(UART0_BASE, '\t');
    UARTCharPut(UART0_BASE, '|');
    UARTCharPut(UART0_BASE, '\t');
    UARTCharPut(UART0_BASE, board[1][1]);
    UARTCharPut(UART0_BASE, '\t');
    UARTCharPut(UART0_BASE, '|');
    UARTCharPut(UART0_BASE, '\t');
    UARTCharPut(UART0_BASE, board[1][2]);
    UARTCharPut(UART0_BASE, '\t');
    UARTCharPut(UART0_BASE, '|');
    UARTCharPut(UART0_BASE, '\t');
    UARTCharPut(UART0_BASE, '\n');
    UARTCharPut(UART0_BASE, '\r');
    UARTCharPut(UART0_BASE, board[2][0]);
    UARTCharPut(UART0_BASE, '\t');
    UARTCharPut(UART0_BASE, '|');
    UARTCharPut(UART0_BASE, '\t');
    UARTCharPut(UART0_BASE, board[2][1]);
    UARTCharPut(UART0_BASE, '\t');
    UARTCharPut(UART0_BASE, '|');
    UARTCharPut(UART0_BASE, '\t');
    UARTCharPut(UART0_BASE, board[2][2]);
    UARTCharPut(UART0_BASE, '\t');
    UARTCharPut(UART0_BASE, '|');
    UARTCharPut(UART0_BASE, '\n');
    UARTCharPut(UART0_BASE, '\r');
}

void indexMap() //print index map to offer choices
{
    UARTCharPut(UART0_BASE, '1');
    UARTCharPut(UART0_BASE, ' ');
    UARTCharPut(UART0_BASE, '|');
    UARTCharPut(UART0_BASE, ' ');
    UARTCharPut(UART0_BASE, '2');
    UARTCharPut(UART0_BASE, ' ');
    UARTCharPut(UART0_BASE, '|');
    UARTCharPut(UART0_BASE, ' ');
    UARTCharPut(UART0_BASE, '3');
    UARTCharPut(UART0_BASE, ' ');
    UARTCharPut(UART0_BASE, '|');
    UARTCharPut(UART0_BASE, ' ');
    UARTCharPut(UART0_BASE, '\n');
    UARTCharPut(UART0_BASE, '\r');
    UARTCharPut(UART0_BASE, '4');
    UARTCharPut(UART0_BASE, ' ');
    UARTCharPut(UART0_BASE, '|');
    UARTCharPut(UART0_BASE, ' ');
    UARTCharPut(UART0_BASE, '5');
    UARTCharPut(UART0_BASE, ' ');
    UARTCharPut(UART0_BASE, '|');
    UARTCharPut(UART0_BASE, ' ');
    UARTCharPut(UART0_BASE, '6');
    UARTCharPut(UART0_BASE, ' ');
    UARTCharPut(UART0_BASE, '|');
    UARTCharPut(UART0_BASE, ' ');
    UARTCharPut(UART0_BASE, '\n');
    UARTCharPut(UART0_BASE, '\r');
    UARTCharPut(UART0_BASE, '7');
    UARTCharPut(UART0_BASE, ' ');
    UARTCharPut(UART0_BASE, '|');
    UARTCharPut(UART0_BASE, ' ');
    UARTCharPut(UART0_BASE, '8');
    UARTCharPut(UART0_BASE, ' ');
    UARTCharPut(UART0_BASE, '|');
    UARTCharPut(UART0_BASE, ' ');
    UARTCharPut(UART0_BASE, '9');
    UARTCharPut(UART0_BASE, ' ');
    UARTCharPut(UART0_BASE, '|');
    UARTCharPut(UART0_BASE, ' ');
    UARTCharPut(UART0_BASE, '\n');
    UARTCharPut(UART0_BASE, '\r');
}

//checks if there is a winner. return values -> 1: xwon. 2: ywon. 3: draw.
int winner(char board[3][3])
{
    if (board[0][0] == 'x' && board[0][1] == 'x' && board[0][2] == 'x') return 1;
    if (board[1][0] == 'x' && board[1][1] == 'x' && board[1][2] == 'x') return 1;
    if (board[2][0] == 'x' && board[2][1] == 'x' && board[2][2] == 'x') return 1;
    if (board[0][0] == 'x' && board[1][0] == 'x' && board[2][0] == 'x') return 1;
    if (board[0][1] == 'x' && board[1][1] == 'x' && board[2][1] == 'x') return 1;
    if (board[0][2] == 'x' && board[1][2] == 'x' && board[2][2] == 'x') return 1;
    if (board[0][0] == 'x' && board[1][1] == 'x' && board[2][2] == 'x') return 1;
    if (board[0][2] == 'x' && board[1][1] == 'x' && board[2][0] == 'x') return 1;
    if (board[0][0] == 'o' && board[0][1] == 'o' && board[0][2] == 'o') return 2;
    if (board[1][0] == 'o' && board[1][1] == 'o' && board[1][2] == 'o') return 2;
    if (board[2][0] == 'o' && board[2][1] == 'o' && board[2][2] == 'o') return 2;
    if (board[0][0] == 'o' && board[1][0] == 'o' && board[2][0] == 'o') return 2;
    if (board[0][1] == 'o' && board[1][1] == 'o' && board[2][1] == 'o') return 2;
    if (board[0][2] == 'o' && board[1][2] == 'o' && board[2][2] == 'o') return 2;
    if (board[0][0] == 'o' && board[1][1] == 'o' && board[2][2] == 'o') return 2;
    if (board[0][2] == 'o' && board[1][1] == 'o' && board[2][0] == 'o') return 2;

    int i = 0, j = 0;
    for (i = 0; i < 3; i++)
    {
        for (j = 0; j < 3; j++)
        {
            if (board[i][j] == 0) return 0;
        }
    }
    return 3;
}

