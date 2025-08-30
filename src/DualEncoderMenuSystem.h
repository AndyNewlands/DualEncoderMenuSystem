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

class BaseMenu
{
protected:
    MENU_ITEM_TYPE type = MENU_ITEM_TYPE::NONE;

    static int dispHeight;
    static int dispWidth;
    static LiquidCrystal_I2C *lcd;
    static RotaryEncoder *encoderA;
    static RotaryEncoder *encoderB;
    static bool initialised;

    BaseMenu *prevMenu = nullptr;
    char typeIndicatorChar = 0x7E; // Indicates action (up arrow (\001 return) = return, down arrow (\002 enter) = enter menu/function, right arrow (\x7E ->) = edit value)

public:
    char *dispText = nullptr;

    static void encoderAturned(long value);
    static void encoderApressed(unsigned long value);
    static void encoderBturned(long value);
    static void encoderBpressed(unsigned long value);
    static void init(int dispWidth, int dispHeight, LiquidCrystal_I2C *lcd, RotaryEncoder *encoderA, RotaryEncoder *encoderB);

    BaseMenu(char *dispText);
    virtual void display(int row, bool select);
    virtual void displayValue();
    virtual void takeFocus();
    virtual void returnFocus(ENCODER_SOURCE source, ENCODER_EVENT event, unsigned long value);
    virtual void retakeFocus(BaseMenu *returningMenu, ENCODER_SOURCE source, ENCODER_EVENT event, unsigned long value);
    virtual void inputHandler(ENCODER_SOURCE source, ENCODER_EVENT event, unsigned long value) {};
};

class Menu : public BaseMenu
{
protected:
    BaseMenu **subMenus = nullptr;
    int numSubMenus = 0;
    int selectedIndex = -1;

public:
    Menu(char *dispText, BaseMenu **subMenus, int numSubMenus);
    void displayValue() override;
    void takeFocus() override;
    void retakeFocus(BaseMenu *returningMenu, ENCODER_SOURCE source, ENCODER_EVENT event, unsigned long value) override;
    void inputHandler(ENCODER_SOURCE source, ENCODER_EVENT event, unsigned long value) override;
};

class MenuBoolValue : public BaseMenu
{
protected:
    char *falseOption = nullptr;
    char *trueOption = nullptr;
    bool *value = nullptr;

public:
    MenuBoolValue(char *dispText, char *falseOption, char *trueOption, bool *value);
    void displayValue() override;
    void takeFocus() override;
    void inputHandler(ENCODER_SOURCE source, ENCODER_EVENT event, unsigned long value) override;
};

class MenuLongValue : public BaseMenu
{
protected:
    char *units = nullptr;
    long *value = nullptr;
    long minValue = 0;
    long maxValue = 0;

public:
    MenuLongValue(char *dispText, char *units, long minValue, long maxValue, long *value);
    void displayValue() override;
    void inputHandler(ENCODER_SOURCE source, ENCODER_EVENT event, unsigned long value) override;
};

class MenuSmallFloatValue : public BaseMenu
{
protected:
    char *units = nullptr;
    float *value = nullptr;
    float minValue = 0.0;
    float maxValue = 0.0;

public:
    MenuSmallFloatValue(char *dispText, char *units, float minValue, float maxValue, float *value);
    void displayValue() override;
    void inputHandler(ENCODER_SOURCE source, ENCODER_EVENT event, unsigned long value) override;
};

class MenuDropDownListValue : public BaseMenu
{
protected:
    int *value = nullptr;
    char **list = nullptr;
    int listSize = 0;

public:
    MenuDropDownListValue(char *dispText, char **list, int listSize, int *value);
    void displayValue() override;
    void takeFocus() override;
    void inputHandler(ENCODER_SOURCE source, ENCODER_EVENT event, unsigned long value) override;
};

class MenuRotaryListValue : public BaseMenu
{
protected:
    int row = 0;
    bool selected = false;
    int *value = nullptr;
    char **list = nullptr;
    int listSize = 0;

public:
    MenuRotaryListValue(char *dispText, char **list, int listSize, int *value);
    void display(int row, bool select) override;
    void displayValue() override;
    void takeFocus() override;
    void inputHandler(ENCODER_SOURCE source, ENCODER_EVENT event, unsigned long value) override;
};

typedef void (*action_function_t)(BaseMenu *, ENCODER_SOURCE, ENCODER_EVENT, unsigned long, BaseMenu *);
typedef void (*input_handler_function_t)(ENCODER_SOURCE, ENCODER_EVENT, unsigned long, BaseMenu *);

class MenuAction : public BaseMenu
{
protected:
    action_function_t function = nullptr;
    input_handler_function_t inputHandlerFunction = nullptr;

public:
    MenuAction(char *dispText, action_function_t function, input_handler_function_t inputHandlerFunction);
    void takeFocus() override;
    void retakeFocus(BaseMenu *returningMenu, ENCODER_SOURCE source, ENCODER_EVENT event, unsigned long value) override;
    void inputHandler(ENCODER_SOURCE source, ENCODER_EVENT event, unsigned long value) override;
};

#endif // DUAL_ENCODER_MENU_SYSTEM_H