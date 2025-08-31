#include "DualEncoderMenuSystem.h"

// Define static members
int BaseMenu::dispHeight = 0;
int BaseMenu::dispWidth = 0;
LiquidCrystal_I2C *BaseMenu::lcd = nullptr;
RotaryEncoder *BaseMenu::encoderA = nullptr;
RotaryEncoder *BaseMenu::encoderB = nullptr;

bool BaseMenu::initialised = false;
BaseMenu *currentMenu = nullptr;

char *naStr = (char *)"N/A";
char selectionChar = '>';

byte returnSymbol[] = {
    0b00100,
    0b01110,
    0b11111,
    0b00100,
    0b00100,
    0b11100,
    0b00000,
    0b00000}; // Custom character for return type indicator

byte enterSymbol[] = {
    0b00000,
    0b00000,
    0b11100,
    0b00100,
    0b00100,
    0b11111,
    0b01110,
    0b00100}; // Custom character for enter type indicator

byte rotateSymbol[] = {
    0b00100,
    0b00010,
    0b11111,
    0b00000,
    0b11111,
    0b01000,
    0b00100,
    0b00000}; // Custom character for rotary list type indicator

byte sparkSymbol[] = {
    0b00001,
    0b00010,
    0b00100,
    0b01111,
    0b11110,
    0b00100,
    0b01000,
    0b10000}; // Custom character for action/function type indicator

void BaseMenu::encoderAturned(long value)
{
    Serial.println("Encoder A turned");
    if (currentMenu)
        currentMenu->inputHandler(ENCODER_SOURCE::A, ENCODER_EVENT::TURNED, value);
}

void BaseMenu::encoderApressed(unsigned long value)
{
    Serial.println("Encoder A pressed");
    if (currentMenu)
        currentMenu->inputHandler(ENCODER_SOURCE::A, ENCODER_EVENT::PRESSED, value);
}

void BaseMenu::encoderBturned(long value)
{
    Serial.println("Encoder B turned");
    if (currentMenu)
        currentMenu->inputHandler(ENCODER_SOURCE::B, ENCODER_EVENT::TURNED, value);
}

void BaseMenu::encoderBpressed(unsigned long value)
{
    Serial.println("Encoder B pressed");
    if (currentMenu)
        currentMenu->inputHandler(ENCODER_SOURCE::B, ENCODER_EVENT::PRESSED, value);
}

void BaseMenu::begin(int displayWidth, int displayHeight, LiquidCrystal_I2C *display, RotaryEncoder *Aencoder, RotaryEncoder *Bencoder)
{
    dispWidth = displayWidth;
    dispHeight = displayHeight;
    lcd = display;
    encoderA = Aencoder;
    encoderB = Bencoder;

    if (!lcd || !encoderA || !encoderB)
        return; // Can't initialise without these

    encoderA->onTurned(&BaseMenu::encoderAturned);
    encoderA->onPressed(&BaseMenu::encoderApressed);
    encoderB->onTurned(&BaseMenu::encoderBturned);
    encoderB->onPressed(&BaseMenu::encoderBpressed);
    lcd->init();
    lcd->backlight();
    lcd->createChar(1, returnSymbol);
    lcd->createChar(2, enterSymbol);
    lcd->createChar(3, rotateSymbol);
    lcd->createChar(4, sparkSymbol);
    lcd->setCursor(0, 0);

    initialised = true;
}

BaseMenu::BaseMenu(const char *dispText)
{
    this->type = MENU_ITEM_TYPE::NONE;
    if (!dispText || ! strlen(dispText))
        dispText = (char *)naStr;

    if (strlen(dispText) <= 14)
        this->dispText =(char *) dispText;
    else
    {
        // Try to AVOID using overly long strings as this will INCREASE RAM usage
        // by creating multiple copies of the string in RAM.  These will NOT be freed
        // until the menu system is destroyed (which is never in the current implementation).
        // That's because menu objects need to be permanent (not constucted on the stack) so that
        // they remain valid while the menu system is in use.
        this->dispText = (char *)malloc(15 * sizeof(char));
        strncpy(this->dispText, dispText, 14);
        this->dispText[14] = 0;
    }
}

void BaseMenu::display(int row, bool select)
{
    char outputText[17];
    sprintf(outputText, "%c%-14s%c", select ? '>' : ' ', dispText, select ? typeIndicator : ' ');
    lcd->setCursor(0, row);
    lcd->print(outputText);
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

void BaseMenu::returnFocus(ENCODER_SOURCE source, ENCODER_EVENT event, unsigned long value)
{
    if (prevMenu == nullptr)
        return; // No previous menu to return to
    prevMenu->retakeFocus(this, source, event, value);
}

void BaseMenu::retakeFocus(BaseMenu *returningMenu, ENCODER_SOURCE source, ENCODER_EVENT event, unsigned long value)
{
    currentMenu = this;
    displayValue();
}

Menu::Menu(const char *dispText, BaseMenu **menuItems) : BaseMenu(dispText)
{
    this->menuItems = menuItems;
    for (itemCount = 0; this->menuItems[itemCount] != nullptr; itemCount++)
        ;
    this->type = MENU_ITEM_TYPE::MENU;
    typeIndicator = '\002'; // Down arrow indicates submenu
}

void Menu::displayValue()
{
    char outputText[17];
    int startIndex, maxIndex, i, row = 0;

    if (!prevMenu && selectedIndex == -1)
        selectedIndex = 0; // No previous menu, so can't return, start at first item
    switch (selectedIndex)
    {
    case -1:
        startIndex = 0;
        maxIndex = 0;
        if (prevMenu)
            sprintf(outputText, "%c%-14s\001", selectionChar, prevMenu->dispText);
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
        menuItems[i]->display(row++, i == selectedIndex);
}

void Menu::takeFocus()
{
    prevMenu = currentMenu;
    currentMenu = this;
    selectedIndex = 0;
    lcd->clear();
    lcd->setCursor(0, 0);
    displayValue();
}

void Menu::retakeFocus(BaseMenu *returningMenu, ENCODER_SOURCE source, ENCODER_EVENT event, unsigned long value)
{
    BaseMenu::retakeFocus(returningMenu, source, event, value);
    if (event == ENCODER_EVENT::TURNED)
        inputHandler(source, event, value); // Pass on the turn event to change selection
}

void Menu::inputHandler(ENCODER_SOURCE source, ENCODER_EVENT event, unsigned long value)
{
    if (event == ENCODER_EVENT::PRESSED)
    {
        // Select submenu
        if (selectedIndex >= 0 && selectedIndex < itemCount)
            menuItems[selectedIndex]->takeFocus();
        else if (selectedIndex == -1)
            returnFocus(source, event, value);
    }
    else if (event == ENCODER_EVENT::TURNED)
    {
        // Change selected index
        if (menuItems && itemCount > 0)
        {
            selectedIndex += value == 1 ? 1 : -1;
            if (prevMenu)
            {
                if (selectedIndex < -1)
                    selectedIndex = -1;
            }
            else
            {
                if (selectedIndex < 0)
                    selectedIndex = 0;
            }
            if (selectedIndex >= itemCount)
                selectedIndex = itemCount - 1;
            // Display menu with new selection
            displayValue();
        }
    }
}

MenuBoolValue::MenuBoolValue(const char *dispText, const char *falseOption, const char *trueOption, bool *value) : BaseMenu(dispText)
{
    if (!falseOption || !strlen(falseOption))
        falseOption = (char *)naStr;
    else
        this->falseOption = (char *)falseOption;
    if (!trueOption || !strlen(trueOption))
        trueOption = (char *)naStr;
    else
        this->trueOption = (char *)trueOption;
    this->value = value;
    this->type = MENU_ITEM_TYPE::BOOL_VALUE;
}

void MenuBoolValue::displayValue()
{
    char fmt[17];
    char trueText[17];
    char outputText[17];
    int lenFalse;

    if (lcd && value)
    {
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
    BaseMenu::takeFocus();
    lcd->setCursor(15, 0);
    lcd->print("\001"); // 1 is the return symbol
}

void MenuBoolValue::inputHandler(ENCODER_SOURCE source, ENCODER_EVENT event, unsigned long value)
{
    if (event == ENCODER_EVENT::PRESSED)
    {
        // Exit & return control to parent
        returnFocus(source, event, value);
    }
    else if (event == ENCODER_EVENT::TURNED)
    {
        // Change value
        if (this->value)
        {
            *(this->value) = value == 1;
            displayValue();
        }
    }
}

MenuLongValue::MenuLongValue(const char *dispText, const char *units, long minValue, long maxValue, long coarseStep, long fineStep, long *value) : BaseMenu(dispText)
{
    this->minValue = min(minValue, maxValue);
    this->maxValue = max(minValue, maxValue);
    this->coarseStep = coarseStep > 0 ? coarseStep : 100;
    this->fineStep = fineStep > 0 ? fineStep : 1;

    if (units)
        this->units = (char *)units;
    else
        this->units = (char *)"";
    this->value = value;
    this->type = MENU_ITEM_TYPE::LONG_VALUE;
}

void MenuLongValue::displayValue()
{
    char valStr[17];
    char outputText[17];
    if (lcd && value)
    {
        sprintf(valStr, "%ld %s", *value, units ? units : "");
        sprintf(outputText, "%-15s\001", valStr); // 1 is the return symbol
        lcd->setCursor(0, 1);
        lcd->print(outputText);
    }
}

void MenuLongValue::inputHandler(ENCODER_SOURCE source, ENCODER_EVENT event, unsigned long value)
{
    if (event == ENCODER_EVENT::PRESSED)
    {
        // Exit menu
        returnFocus(source, event, value);
    }
    else if (event == ENCODER_EVENT::TURNED)
    {
        // Change value
        if (this->value)
        {
            if (source == ENCODER_SOURCE::A)
            {
                *(this->value) += value == 1 ? coarseStep : 0 - coarseStep;
            }
            else
            {
                *(this->value) += value == 1 ? fineStep : 0 - fineStep;
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

MenuSmallFloatValue::MenuSmallFloatValue(const char *dispText, const char *units, float minValue, float maxValue, float *value) : BaseMenu(dispText)
{
    this->minValue = min(minValue, maxValue);
    this->maxValue = max(minValue, maxValue);
    this->units = (char *)units;
    this->value = value;
    this->type = MENU_ITEM_TYPE::SMALL_FLOAT_VALUE;
}

void MenuSmallFloatValue::displayValue()
{
    char valStr[17];
    char outputText[17];
    sprintf(valStr, "%0.3f %s", *value, units ? units : "");
    sprintf(outputText, "%-14s \001", valStr); // 1 is the return symbol
    lcd->setCursor(0, 1);
    lcd->print(outputText);
}

void MenuSmallFloatValue::inputHandler(ENCODER_SOURCE source, ENCODER_EVENT event, unsigned long value)
{
    if (event == ENCODER_EVENT::PRESSED)
    {
        // Exit menu
        returnFocus(source, event, value);
    }
    else if (event == ENCODER_EVENT::TURNED)
    {
        // Change value
        if (this->value)
        {
            if (source == ENCODER_SOURCE::A)
            {
                *(this->value) += value == 1 ? 0.05 : -0.05;
            }
            else
            {
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

MenuDropDownListValue::MenuDropDownListValue(const char *dispText, const char **listItems, int *value) : BaseMenu(dispText)
{
    this->listItems = (char **)listItems;
    for (itemCount = 0; listItems[itemCount] != nullptr; itemCount++)
        ;
    this->value = value;
    this->type = MENU_ITEM_TYPE::DROP_DOWN_LIST_VALUE;

    char temp[15];
    for (int i = 0; i < itemCount; i++)
    {
        if (strlen(this->listItems[i]) > 14)
        {
            // Try to AVOID using overly long strings as this will INCREASE RAM usage
            // by creating multiple copies of the string in RAM.  These will NOT be freed
            // until the menu system is destroyed (which is never in the current implementation).
            // That's because menu objects need to be permanent (not constucted on the stack) so that
            // they remain valid while the menu system is in use.
            memcpy(temp, listItems[i], 14 * sizeof(char));
            temp[14] = 0;                                      // Truncate to 14 characters
            this->listItems[i] = (char *)malloc(15 * sizeof(char)); // Allocate 15 bytes (14 chars + null terminator)
            memcpy(this->listItems[i], temp, 15 * sizeof(char));
        }
        else
            this->listItems[i] = (char *)listItems[i];
    }
}

void MenuDropDownListValue::displayValue()
{
    char outputText[17];
    int index = *value;
    if (index < 0)
        index = 0;
    if (index >= itemCount)
        index = itemCount - 1;
    sprintf(outputText, "%c%-15s", selectionChar, listItems[index]);
    lcd->setCursor(0, 1);
    lcd->print(outputText);
}

void MenuDropDownListValue::takeFocus()
{
    BaseMenu::takeFocus();
    lcd->setCursor(15, 0);
    lcd->print("\001"); // 1 is the return symbol
}

void MenuDropDownListValue::inputHandler(ENCODER_SOURCE source, ENCODER_EVENT event, unsigned long value)
{
    if (event == ENCODER_EVENT::PRESSED)
    {
        // Exit menu
        returnFocus(source, event, value);
    }
    else if (event == ENCODER_EVENT::TURNED)
    {
        // Change value
        if (this->value && listItems && itemCount > 0)
        {
            *(this->value) += value == 1 ? 1 : -1;
            if (*(this->value) < 0)
                *(this->value) = 0;
            else if (*(this->value) >= itemCount)
                *(this->value) = itemCount - 1;
            displayValue();
        }
    }
}

MenuRotaryListValue::MenuRotaryListValue(const char *dispText, const char **listItems, int *value) : BaseMenu(dispText)
{
    this->listItems = (char **)listItems;
    for (itemCount = 0; this->listItems[itemCount] != nullptr; itemCount++)
        ;
    this->value = value;
    this->type = MENU_ITEM_TYPE::ROTARY_LIST_VALUE;

    char temp[15];
    for (int i = 0; i < itemCount; i++)
    {
        if (strlen(this->listItems[i]) > 14)
        {
            // Try to AVOID using overly long strings as this will INCREASE RAM usage
            // by creating multiple copies of the string in RAM.  These will NOT be freed
            // until the menu system is destroyed (which is never in the current implementation).
            // That's because menu objects need to be permanent (not constucted on the stack) so that
            // they remain valid while the menu system is in use.
            memcpy(temp, listItems[i], 14 * sizeof(char));
            temp[14] = 0;
            this->listItems[i] = (char *)malloc(15 * sizeof(char));
            memcpy(this->listItems[i], temp, 15 * sizeof(char));
        }
        else
            this->listItems[i] = (char *)listItems[i];
    }
    typeIndicator = '\003'; // Rotary symbol indicates rotary selection
}

void MenuRotaryListValue::display(int row, bool select)
{
    this->row = row;
    this->selected = select;
    displayValue();
}

void MenuRotaryListValue::displayValue()
{
    char outputText[17];
    if (*value < 0)
        *value = 0;
    if (*value >= itemCount)
        *value = itemCount - 1;
    sprintf(outputText, "%c%-14s%c", selected ? '>' : ' ', listItems[*value], selected ? typeIndicator : ' ');
    lcd->setCursor(0, this->row);
    lcd->print(outputText);
}

void MenuRotaryListValue::takeFocus()
{
    prevMenu = currentMenu;
    currentMenu = this;

    this->inputHandler(ENCODER_SOURCE::A, ENCODER_EVENT::PRESSED, 1000); // Force display of value
}

void MenuRotaryListValue::inputHandler(ENCODER_SOURCE source, ENCODER_EVENT event, unsigned long value)
{
    if (event == ENCODER_EVENT::TURNED)
    {
        // Exit menu
        returnFocus(source, event, value);
    }
    else if (event == ENCODER_EVENT::PRESSED)
    {
        // Change value
        if (this->value && listItems && itemCount > 0)
        {
            *(this->value) += 1;
            if (*(this->value) >= itemCount)
                *(this->value) = 0;
            displayValue();
        }
    }
}

typedef void (*action_function_t)(BaseMenu *, ENCODER_SOURCE, ENCODER_EVENT, unsigned long, BaseMenu *);
typedef void (*input_handler_function_t)(ENCODER_SOURCE, ENCODER_EVENT, unsigned long, BaseMenu *);
MenuAction::MenuAction(const char *dispText, action_function_t function, input_handler_function_t inputHandlerFunction) : BaseMenu(dispText)
{
    this->dispText = (char *)dispText;
    this->function = function;
    this->inputHandlerFunction = inputHandlerFunction;
    this->type = MENU_ITEM_TYPE::FUNCTION;
    typeIndicator = '\004';
}

void MenuAction::takeFocus()
{
    prevMenu = currentMenu;
    currentMenu = this;

    function(this, ENCODER_SOURCE::A, ENCODER_EVENT::PRESSED, 0, nullptr); // Call the function associated with this menu item (indicate we just took focus)
}

void MenuAction::retakeFocus(BaseMenu *returningMenu, ENCODER_SOURCE source, ENCODER_EVENT event, unsigned long value)
{
    currentMenu = this;
    function(this, source, event, value, returningMenu); // Call the function associated with this menu item (indicate we are retaking focus)
}

void MenuAction::inputHandler(ENCODER_SOURCE source, ENCODER_EVENT event, unsigned long value)
{
    if (inputHandlerFunction)
        inputHandlerFunction(source, event, value, this);
    else if (event == ENCODER_EVENT::PRESSED)
        // Exit menu
        returnFocus(source, event, value);
}
