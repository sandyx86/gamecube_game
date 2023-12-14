#pragma once

#include <gccore.h>
#include <stdarg.h>

int controllers = 1;

void nullfunc(){return;}

//function pointers
typedef struct inputMap {
    void (*buttonA)();
    void (*buttonB)();
    void (*buttonX)();
    void (*buttonY)();
    void (*buttonUp)();
    void (*buttonDown)();
    void (*buttonLeft)();
    void (*buttonRight)();
    void (*buttonStart)();
    void (*triggerL)();
    void (*triggerR)();
    void (*triggerZ)();
    void **parameter;
} InputMap;

InputMap imap;
//might be able to make an array of input maps
//to support multiple controllers later

//init all the func pointers so it doesn't crash
void initHandler() {
    imap.buttonA = nullfunc;     
    imap.buttonB = nullfunc;     
    imap.buttonX = nullfunc;     
    imap.buttonY = nullfunc;     
    imap.buttonUp = nullfunc;    
    imap.buttonDown = nullfunc;  
    imap.buttonLeft = nullfunc;  
    imap.buttonRight = nullfunc; 
    imap.buttonStart = nullfunc; 
    imap.triggerL = nullfunc;    
    imap.triggerR = nullfunc;    
    imap.triggerZ = nullfunc;    
}

//super readable switch statement
void assignFunction(int button, void (*func)()) {
    switch (button) {
    case PAD_BUTTON_A:      imap.buttonA = func;        break;
    case PAD_BUTTON_B:      imap.buttonB = func;        break;
    case PAD_BUTTON_X:      imap.buttonX = func;        break;
    case PAD_BUTTON_Y:      imap.buttonY = func;        break;
    case PAD_BUTTON_UP:     imap.buttonUp = func;       break;
    case PAD_BUTTON_DOWN:   imap.buttonDown = func;     break;
    case PAD_BUTTON_LEFT:   imap.buttonLeft = func;     break;
    case PAD_BUTTON_RIGHT:  imap.buttonRight = func;    break;
    case PAD_BUTTON_START:  imap.buttonStart = func;    break;
    case PAD_TRIGGER_L:     imap.triggerL = func;       break;
    case PAD_TRIGGER_R:     imap.triggerR = func;       break;
    case PAD_TRIGGER_Z:     imap.triggerZ = func;       break;
    default: return;
    }
}

//maybe make the input handler into a state machine
//to handle button combos
#define or(button) else if (PAD_ButtonsDown(0) & button)
//input handler
void handleInput() {
    //only checks first controller for now
    //PAD_ScanPads();
    if (PAD_ButtonsDown(0) & PAD_BUTTON_A) {imap.buttonA();}
    or (PAD_BUTTON_B)       {imap.buttonB();}
    or (PAD_BUTTON_X)       {imap.buttonX();}
    or (PAD_BUTTON_Y)       {imap.buttonY();}
    or (PAD_BUTTON_UP)      {imap.buttonUp();}
    or (PAD_BUTTON_DOWN)    {imap.buttonDown();}
    or (PAD_BUTTON_LEFT)    {imap.buttonLeft();}
    or (PAD_BUTTON_RIGHT)   {imap.buttonRight();}
    or (PAD_BUTTON_START)   {imap.buttonStart();}
    or (PAD_TRIGGER_L)      {imap.triggerL();}
    or (PAD_TRIGGER_R)      {imap.triggerR();}
    or (PAD_TRIGGER_Z)      {imap.triggerZ();}
}
#undef or


//try this weird thing
/*
send(to, from, count)
//register short *to, *from;
//register count;
{
    register n = (count + 7) / 8;
    switch (count % 8) {
    case 0: do { *to = *from++;
    case 7:      *to = *from++;
    case 6:      *to = *from++;
    case 5:      *to = *from++;
    case 4:      *to = *from++;
    case 3:      *to = *from++;
    case 2:      *to = *from++;
    case 1:      *to = *from++;
            } while (--n > 0);
    }
}
*/