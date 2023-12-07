#pragma once

#include <gccore.h>

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
    void *parameter;
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

//input handler
void handleInput() {
    //only checks first controller for now
    //maybe don't want to break, incase of simultaneous inputs
    //PAD_ScanPads();
    switch (PAD_ButtonsHeld(0)) {
    case PAD_BUTTON_A:      imap.buttonA();     break;
    case PAD_BUTTON_B:      imap.buttonB();     break;
    case PAD_BUTTON_X:      imap.buttonX();     break;
    case PAD_BUTTON_Y:      imap.buttonY();     break;
    case PAD_BUTTON_UP:     imap.buttonUp();    break;
    case PAD_BUTTON_DOWN:   imap.buttonDown();  break;
    case PAD_BUTTON_LEFT:   imap.buttonLeft();  break;
    case PAD_BUTTON_RIGHT:  imap.buttonRight(); break;
    case PAD_BUTTON_START:  imap.buttonStart(); break;
    case PAD_TRIGGER_L:     imap.triggerL();    break;
    case PAD_TRIGGER_R:     imap.triggerR();    break;
    case PAD_TRIGGER_Z:     imap.triggerZ();    break;
    default: return;
    }
}