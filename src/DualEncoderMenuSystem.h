#ifndef DUAL_ENCODER_MENU_SYSTEM_H
#define DUAL_ENCODER_MENU_SYSTEM_H

#include <Arduino.h>
#include <LiquidCrystal_I2C.h>
#include <ESP32RotaryEncoder.h>

enum ENCODER_SOURCE
{
    A,
    B
};

enum ENCODER_EVENT
{
    TURNED,
    PRESSED
};

enum MENU_ITEM_TYPE
{
    NONE,
    MENU,
    FUNCTION,
    BOOL_VALUE,
    LONG_VALUE,
    SMALL_FLOAT_VALUE,
    DROP_DOWN_LIST_VALUE,
    ROTARY_LIST_VALUE
};

class MenuSystem
{
protected:
    MENU_ITEM_TYPE type = MENU_ITEM_TYPE::NONE;

    static int dispHeight;
    static int dispWidth;
    static LiquidCrystal_I2C *lcd;
    static RotaryEncoder *encoderA;
    static RotaryEncoder *encoderB;
    static bool initialised;

    MenuSystem *prevMenu = nullptr;
    char typeIndicator = 0x7E; // Indicates action (up arrow (\001 return) = return, down arrow (\002 enter) = enter menu/function, right arrow  (->) = edit value)

public:
    char *dispText = nullptr;

    static void encoderAturned(long value);
    static void encoderApressed(unsigned long value);
    static void encoderBturned(long value);
    static void encoderBpressed(unsigned long value);
    static void begin(int dispWidth, int dispHeight, LiquidCrystal_I2C *lcd, RotaryEncoder *encoderA, RotaryEncoder *encoderB);

    MenuSystem(const char *dispText);
    virtual void display(int row, bool select);
    virtual void displayValue();
    virtual void takeFocus();
    virtual void returnFocus(ENCODER_SOURCE source, ENCODER_EVENT event, unsigned long value);
    virtual void retakeFocus(MenuSystem *returningMenu, ENCODER_SOURCE source, ENCODER_EVENT event, unsigned long value);
    virtual void inputHandler(ENCODER_SOURCE source, ENCODER_EVENT event, unsigned long value) {};
};

class Menu : public MenuSystem
{
protected:
    MenuSystem **menuItems = nullptr;
    int itemCount = 0;
    int selectedIndex = -1;

public:
    Menu(const char *dispText, MenuSystem **menuItems);
    void displayValue() override;
    void takeFocus() override;
    void retakeFocus(MenuSystem *returningMenu, ENCODER_SOURCE source, ENCODER_EVENT event, unsigned long value) override;
    void inputHandler(ENCODER_SOURCE source, ENCODER_EVENT event, unsigned long value) override;
};

class MenuBoolValue : public MenuSystem
{
protected:
    char *falseOption = nullptr;
    char *trueOption = nullptr;
    bool *value = nullptr;

public:
    MenuBoolValue(const char *dispText, const char *falseOption, const char *trueOption, bool *value);
    void displayValue() override;
    void takeFocus() override;
    void inputHandler(ENCODER_SOURCE source, ENCODER_EVENT event, unsigned long value) override;
};

class MenuLongValue : public MenuSystem
{
protected:
    char *units = nullptr;
    long *value = nullptr;
    long minValue = 0;
    long maxValue = 0;
    long coarseStep = 100;
    long fineStep = 1;

public:
    MenuLongValue(const char *dispText, const char *units, long minValue, long maxValue, long coarseStep, long fineStep, long *value);
    void displayValue() override;
    void inputHandler(ENCODER_SOURCE source, ENCODER_EVENT event, unsigned long value) override;
};

class MenuFloatValue : public MenuSystem
{
protected:
    char *units = nullptr;
    float *value = nullptr;
    float minValue = 0.0;
    float maxValue = 0.0;
    float coarseStep = 0.01;
    float fineStep = 0.001;

public:
    MenuFloatValue(const char *dispText, const char *units, float minValue, float maxValue, float coarseStep, float fineStep, float *value);
    void displayValue() override;
    void inputHandler(ENCODER_SOURCE source, ENCODER_EVENT event, unsigned long value) override;
};

class MenuDropDownListValue : public MenuSystem
{
protected:
    int *value = nullptr;
    char **listItems = nullptr;
    int itemCount = 0;

public:
    MenuDropDownListValue(const char *dispText, const char **listItems, int *value);
    void displayValue() override;
    void takeFocus() override;
    void inputHandler(ENCODER_SOURCE source, ENCODER_EVENT event, unsigned long value) override;
};

class MenuRotaryListValue : public MenuSystem
{
protected:
    int row = 0;
    bool selected = false;
    int *value = nullptr;
    char **listItems = nullptr;
    int itemCount = 0;

public:
    MenuRotaryListValue(const char *dispText, const char **listItems, int *value);
    void display(int row, bool select) override;
    void displayValue() override;
    void takeFocus() override;
    void inputHandler(ENCODER_SOURCE source, ENCODER_EVENT event, unsigned long value) override;
};

typedef void (*action_function_t)(MenuSystem *, ENCODER_SOURCE, ENCODER_EVENT, unsigned long, MenuSystem *);
typedef void (*input_handler_function_t)(ENCODER_SOURCE, ENCODER_EVENT, unsigned long, MenuSystem *);

class MenuAction : public MenuSystem
{
protected:
    action_function_t function = nullptr;
    input_handler_function_t inputHandlerFunction = nullptr;

public:
    MenuAction(const char *dispText, action_function_t function, input_handler_function_t inputHandlerFunction);
    void takeFocus() override;
    void retakeFocus(MenuSystem *returningMenu, ENCODER_SOURCE source, ENCODER_EVENT event, unsigned long value) override;
    void inputHandler(ENCODER_SOURCE source, ENCODER_EVENT event, unsigned long value) override;
};

#endif // DUAL_ENCODER_MENU_SYSTEM_H