#include "DualEncoderMenuSystem.h"

// Define static members
int BaseMenu::dispHeight = 0;
int BaseMenu::dispWidth = 0;
LiquidCrystal_I2C* BaseMenu::lcd = nullptr;
RotaryEncoder* BaseMenu::encoderA = nullptr;
RotaryEncoder* BaseMenu::encoderB = nullptr;

bool BaseMenu::initialised = false;
BaseMenu *currentMenu = nullptr;

byte returnSymbol[] = {
    0b00100,
    0b01110,
    0b11111,
    0b00100,
    0b00100,
    0b11100,
    0b00000,
    0b00000    
 }; // Custom character for return symbol

 byte enterSymbol[] = {
    0b00000,
    0b00000,
    0b11100,
    0b00100,
    0b00100,
    0b11111,
    0b01110,
    0b00100    
 }; // Custom character for return symbol

void BaseMenu::encoderAturned(long value)
{
    if (currentMenu) currentMenu->inputHandler(ENCODER_SOURCE::A, ENCODER_EVENT::TURNED, value);
}

void BaseMenu::encoderApressed(unsigned long value)
{
    if(currentMenu) currentMenu->inputHandler(ENCODER_SOURCE::A, ENCODER_EVENT::PRESSED, value);
}

void BaseMenu::encoderBturned(long value)
{
    if(currentMenu) currentMenu->inputHandler(ENCODER_SOURCE::B, ENCODER_EVENT::TURNED, value);
}

void BaseMenu::encoderBpressed(unsigned long value)
{
    if(currentMenu) currentMenu->inputHandler(ENCODER_SOURCE::B, ENCODER_EVENT::PRESSED, value);
}

void BaseMenu::init(int displayWidth, int displayHeight, LiquidCrystal_I2C *display, RotaryEncoder *Aencoder, RotaryEncoder *Bencoder)
{
    dispWidth = displayWidth;
    dispHeight = displayHeight;
    lcd = display;
    encoderA = Aencoder;
    encoderB = Bencoder;
    encoderA->onTurned(&BaseMenu::encoderAturned);
    encoderA->onPressed(&BaseMenu::encoderApressed);
    encoderB->onTurned(&BaseMenu::encoderBturned);
    encoderB->onPressed(&BaseMenu::encoderBpressed);
    lcd->init();
    lcd->backlight();
    lcd->createChar(1, returnSymbol);
    lcd->createChar(2, enterSymbol);
    lcd->clear();
    lcd->setCursor(0, 0);
    initialised = true;
}

BaseMenu::BaseMenu(char *dispText)
{
    this->dispText = dispText;
    this->type = MENU_ITEM_TYPE::NONE;
}

void BaseMenu::display(int row, bool select)
{
    if(lcd && dispText) {
        lcd->setCursor(0, row);
        if(select) lcd->print(">");
        else lcd->print(" ");
        lcd->print(dispText);
        int len = strlen(dispText);
        for(int i=len+1; i<dispWidth; i++) {
            lcd->print(" ");
        }
        if (select) {
            lcd->setCursor(15, row);
            lcd->print(selectionSymbol);  // -> Right-arrow indicates value selection or submenu
        }
    }    
}

void BaseMenu::displayValue()
{
    // Default implementation does nothing
}

void BaseMenu::takeFocus()
{
    prevMenu = currentMenu;
    currentMenu = this;

    lcd->clear();
    lcd->setCursor(0, 0);
    lcd->print(dispText);
    displayValue();
}

void BaseMenu::returnFocus()
{
    if (prevMenu == nullptr) return; // No previous menu to return to
    prevMenu->retakeFocus();
}

void BaseMenu::retakeFocus()
{
    currentMenu = this;
    displayValue();
}

Menu::Menu(char *dispText, BaseMenu **subMenus, int numSubMenus) : BaseMenu(dispText)
{
    this->subMenus = subMenus;
    this->numSubMenus = numSubMenus;
    this->dispText = dispText;
    this->type = MENU_ITEM_TYPE::MENU;
    selectionSymbol = "\002"; // Down arrow indicates submenu
}

void Menu::takeFocus()
{
    prevMenu = currentMenu;
    currentMenu = this;
    lcd->clear();
    lcd->setCursor(0, 0);
    displayValue();
}

void Menu::displayValue()
{
    char outputText[17];
    int startIndex, maxIndex, i, row = 0;
    Serial.print("Display menu, selectedIndex="); Serial.println(selectedIndex);
    if (! prevMenu && selectedIndex == -1)
        selectedIndex = 0; // No previous menu, so can't return, start at first item
    switch (selectedIndex) {
    case -1:
        startIndex = 0;
        maxIndex = 0;
        if (prevMenu)
            sprintf(outputText, ">%-14s\001", prevMenu->dispText);
        else
            sprintf(outputText, "%-16s", dispText);
        lcd->setCursor(0, row++);
        lcd->print(outputText);
        break;
    case 0:
        startIndex = 0;
        maxIndex = 0;
        if (prevMenu)
            sprintf(outputText, " %-15s", prevMenu->dispText);
        else
            sprintf(outputText, "%-16s", dispText);
        lcd->setCursor(0, row++);
        lcd->print(outputText);
        break;
    default:
        startIndex = selectedIndex - 1;
        maxIndex = selectedIndex;
        break;
    }
    for (i = startIndex; i <= maxIndex; i++)
        subMenus[i]->display(row++, i == selectedIndex);
}

void Menu::inputHandler(ENCODER_SOURCE source, ENCODER_EVENT event, unsigned long value)
{
    if(event == ENCODER_EVENT::PRESSED) {
        // Select submenu
        if (selectedIndex >= 0 && selectedIndex < numSubMenus)
            subMenus[selectedIndex]->takeFocus();
        else if (selectedIndex == -1)
            returnFocus();
    } else if(event == ENCODER_EVENT::TURNED) {
        // Change selected index
        if (subMenus && numSubMenus > 0) {
            selectedIndex += value == 1 ? 1 : -1;
            if (prevMenu) {
                if (selectedIndex < -1)
                    selectedIndex = -1;
            } else {
                if (selectedIndex < 0)
                    selectedIndex = 0;
            }
            if (selectedIndex >= numSubMenus)
                selectedIndex = numSubMenus - 1;
            // Display menu with new selection
            displayValue();
        }
    }
}

MenuBoolValue::MenuBoolValue(char *dispText, char *falseOption, char *trueOption, bool *value) : BaseMenu(dispText)
{
    this->falseOption = falseOption;
    this->trueOption = trueOption;
    this->value = value;
    this->type = MENU_ITEM_TYPE::BOOL_VALUE;
}

void MenuBoolValue::displayValue()
{
	char fmt[17];
    char trueText[17];
	char outputText[17];
    int lenFalse;

    if(lcd && value) {
        lcd->setCursor(15, 0);
        lcd->print("\001"); // 1 is the return symbol
        // Display options with current value indicated by '>'
        sprintf(trueText, "%c%s", *value == 1 ? '>' : ' ', trueOption);
        lenFalse = strlen(falseOption) + 2;
        sprintf(fmt, "%c%s %%%ds", *value == 0 ? '>' : ' ', falseOption, 16 - lenFalse);
        sprintf(outputText, fmt, trueText);
        lcd->setCursor(0, 1);
        lcd->print(outputText);
    }
}

void MenuBoolValue::takeFocus()
{
    char outputText[17];
    prevMenu = currentMenu;
    currentMenu = this;

    lcd->clear();
    lcd->setCursor(0, 0);
    sprintf(outputText, "%-15s\001", dispText); // 1 is the return symbol
    lcd->print(outputText);
    displayValue();
}

void MenuBoolValue::inputHandler(ENCODER_SOURCE source, ENCODER_EVENT event, unsigned long value)
{
    if(event == ENCODER_EVENT::PRESSED) {
        // Exit & return control to parent
        returnFocus();
    } else if(event == ENCODER_EVENT::TURNED) {
        // Change value
        if(this->value) {
            *(this->value) = value == 1;
            displayValue();
        }
    }
}

MenuLongValue::MenuLongValue(char *dispText, char *units, long minValue, long maxValue, long *value) : BaseMenu(dispText)
{
    this->minValue = min(minValue, maxValue);
    this->maxValue = max(minValue, maxValue);
    this->units = units;
    this->value = value;
    this->type = MENU_ITEM_TYPE::LONG_VALUE;
}

void MenuLongValue::displayValue()
{
    char valStr[17];
    char outputText[17];
    if(lcd && value) {
        sprintf(valStr, "%ld %s", *value, units ? units : "");
        sprintf(outputText, "%-14s \001", valStr); // 1 is the return symbol
        lcd->setCursor(0, 1);
        lcd->print(outputText);
    }
}

void MenuLongValue::inputHandler(ENCODER_SOURCE source, ENCODER_EVENT event, unsigned long value)
{
    if(event == ENCODER_EVENT::PRESSED) {
        // Exit menu
        returnFocus();
    } else if(event == ENCODER_EVENT::TURNED) {
        // Change value
        if(this->value) {
            if (source == ENCODER_SOURCE::A) {
                *(this->value) += value == 1 ? 100 : -100;
            } else {
                *(this->value) += value == 1 ? 1 : -1;
            }
            if (minValue != maxValue)
            {
                if (*(this->value) > maxValue)
                    *(this->value) = maxValue;
                else if (*(this->value) < minValue)
                    *(this->value) = minValue;
            }
            displayValue();
        }
    }
}

MenuSmallFloatValue::MenuSmallFloatValue(char *dispText, char *units, float minValue, float maxValue, float *value) : BaseMenu(dispText)
{
    this->minValue = min(minValue, maxValue);
    this->maxValue = max(minValue, maxValue);
    this->units = units;
    this->value = value;
    this->type = MENU_ITEM_TYPE::SMALL_FLOAT_VALUE;
}

void MenuSmallFloatValue::displayValue()
{
    char valStr[17];
    char outputText[17];
    if(lcd && value) {
        sprintf(valStr, "%0.3f %s", *value, units ? units : "");
        sprintf(outputText, "%-14s \001", valStr); // 1 is the return symbol
        lcd->setCursor(0, 1);
        lcd->print(outputText);
    }
}

void MenuSmallFloatValue::inputHandler(ENCODER_SOURCE source, ENCODER_EVENT event, unsigned long value)
{
    if(event == ENCODER_EVENT::PRESSED) {
        // Exit menu
        returnFocus();
    } else if(event == ENCODER_EVENT::TURNED) {
        // Change value
        if(this->value) {
            if (source == ENCODER_SOURCE::A) {
                *(this->value) += value == 1 ? 0.05 : -0.05;
            } else {
                *(this->value) += value == 1 ? 0.001 : -0.001;
            }
            if (minValue != maxValue)
            {
                if (*(this->value) > maxValue)
                    *(this->value) = maxValue;
                else if (*(this->value) < minValue)
                    *(this->value) = minValue;
            }
            displayValue();
        }
    }
}

MenuListValue::MenuListValue(char *dispText, char **list, int listSize, int *value) : BaseMenu(dispText)
{
    this->list = list;
    this->listSize = listSize;
    this->value = value;
    this->type = MENU_ITEM_TYPE::LIST_VALUE;
}

void MenuListValue::displayValue()
{
    char outputText[17];
    if(lcd && value && list && listSize > 0) {
        int index = *value;
        if (index < 0) index = 0;
        if (index >= listSize) index = listSize - 1;
        sprintf(outputText, ">%-14s\001", list[index]);  // 1 is the return symbol
        lcd->setCursor(0, 1);
        lcd->print(outputText);
    }
}

void MenuListValue::inputHandler(ENCODER_SOURCE source, ENCODER_EVENT event, unsigned long value)
{
    if(event == ENCODER_EVENT::PRESSED) {
        // Exit menu
        returnFocus();
    } else if(event == ENCODER_EVENT::TURNED) {
        // Change value
        if(this->value && list && listSize > 0) {
            *(this->value) += value == 1 ? 1 : -1;
            if (*(this->value) < 0)
                *(this->value) = 0;
            else if (*(this->value) >= listSize)
                *(this->value) = listSize - 1;
            displayValue();
        }
    }
}

MenuFunction::MenuFunction(char *dispText, menu_function_t function, input_handler_function_t inputHandlerFunction) : BaseMenu(dispText)
{
    this->dispText = dispText;
    this->function = function;
    this->inputHandlerFunction = inputHandlerFunction;
    this->type = MENU_ITEM_TYPE::FUNCTION;
    selectionSymbol = "\002"; // Down arrow indicates submenu
}

void MenuFunction::takeFocus()
{
    prevMenu = currentMenu;
    currentMenu = this;

    function(this, true);  // Call the function associated with this menu item (indicate we just took focus)
}

void MenuFunction::retakeFocus()
{
    currentMenu = this;
    function(this, false);  // Call the function associated with this menu item (indicate we are retaking focus)
}

void MenuFunction::inputHandler(ENCODER_SOURCE source, ENCODER_EVENT event, unsigned long value)
{
    if (inputHandlerFunction)
        inputHandlerFunction(source, event, value, this);
    else if (event == ENCODER_EVENT::PRESSED)
        // Exit menu
        returnFocus();
}
