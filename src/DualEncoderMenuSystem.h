#ifndef DUAL_ENCODER_MENU_SYSTEM_H
#define DUAL_ENCODER_MENU_SYSTEM_H

#include <Arduino.h>
#include <LiquidCrystal_I2C.h>
#include <ESP32RotaryEncoder.h>

enum ENCODER_SOURCE {
	A,
	B
};

enum ENCODER_EVENT {
	TURNED,
	PRESSED
};

enum MENU_ITEM_TYPE {
	NONE,
	MENU,
	FUNCTION,
	BOOL_VALUE,
	LONG_VALUE,
	SMALL_FLOAT_VALUE,
    LIST_VALUE
};

class BaseMenu {
protected:
	MENU_ITEM_TYPE type = MENU_ITEM_TYPE::NONE;

	static int dispHeight;
	static int dispWidth;
	static LiquidCrystal_I2C *lcd;
	static RotaryEncoder *encoderA;
	static RotaryEncoder *encoderB;
    static bool initialised;

    char *selectionSymbol = "\x7E"; // Indicates action (up arrow (\001 return) = return, down arrow (\002 enter) = enter menu/function, right arrow (\x7E ->) = edit value)

    BaseMenu *prevMenu = nullptr;

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
    virtual void returnFocus();
    virtual void retakeFocus();
    virtual void inputHandler(ENCODER_SOURCE source, ENCODER_EVENT event, unsigned long value) {};
};

class Menu : public BaseMenu {
protected:
    BaseMenu **subMenus = nullptr;
    int numSubMenus = 0;
    int selectedIndex = -1;

    void displayValue() override;
public:
    Menu(char *dispText, BaseMenu **subMenus, int numSubMenus);
    void takeFocus() override;
    void inputHandler(ENCODER_SOURCE source, ENCODER_EVENT event, unsigned long value) override;
};

class MenuBoolValue : public BaseMenu {
protected:
    char *falseOption = nullptr;
    char *trueOption = nullptr;
    bool *value = nullptr;

    void displayValue() override;
public:
    MenuBoolValue(char *dispText, char * falseOption, char *trueOption, bool *value);
    void takeFocus() override;
    void inputHandler(ENCODER_SOURCE source, ENCODER_EVENT event, unsigned long value) override;
};

class MenuLongValue : public BaseMenu {
protected:
    char *units = nullptr;
    long *value = nullptr;
    long minValue = 0;
    long maxValue = 0;

    void displayValue() override;
public:
    MenuLongValue(char *dispText, char *units, long minValue, long maxValue, long *value);
    void inputHandler(ENCODER_SOURCE source, ENCODER_EVENT event, unsigned long value) override;
};

class MenuSmallFloatValue : public BaseMenu {
protected:
    char *units = nullptr;
    float *value = nullptr;
    float minValue = 0.0;
    float maxValue = 0.0;

    void displayValue() override;
public:
    MenuSmallFloatValue(char *dispText, char *units, float minValue, float maxValue, float *value);
    void inputHandler(ENCODER_SOURCE source, ENCODER_EVENT event, unsigned long value) override;
};

class MenuListValue : public BaseMenu {
protected:
    int *value = nullptr;
    char **list = nullptr;
    int listSize = 0;
    void displayValue() override;
public:
    MenuListValue(char *dispText, char **list, int listSize, int *value);
    void inputHandler(ENCODER_SOURCE source, ENCODER_EVENT event, unsigned long value) override;
};

typedef void(*menu_function_t)(BaseMenu *, bool);
typedef void(*input_handler_function_t)(ENCODER_SOURCE, ENCODER_EVENT, unsigned long, BaseMenu *);

class MenuFunction : public BaseMenu {
protected:
    menu_function_t function = nullptr;
    input_handler_function_t inputHandlerFunction = nullptr;
public:
    MenuFunction(char *dispText, menu_function_t function, input_handler_function_t inputHandlerFunction);
    void takeFocus() override;
    void retakeFocus() override;
    void inputHandler(ENCODER_SOURCE source, ENCODER_EVENT event, unsigned long value) override;
};

#endif // DUAL_ENCODER_MENU_SYSTEM_H