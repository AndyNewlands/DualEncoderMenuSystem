#include <Wire.h> 
#include <ArduinoQueue.h>
#include <MultiStepperLite.h>
#include <LiquidCrystal_I2C.h>
#include <ESP32RotaryEncoder.h>
#include <DualEncoderMenuSystem.h>

#define A_ENCODER_A 21
#define A_ENCODER_B 22
#define A_ENCODER_SW 23

#define B_ENCODER_A 32
#define B_ENCODER_B 33
#define B_ENCODER_SW 34

#define SDA_PIN 18
#define SCL_PIN 19

RotaryEncoder aEncoder( A_ENCODER_A, A_ENCODER_B, A_ENCODER_SW);
RotaryEncoder bEncoder( B_ENCODER_A, B_ENCODER_B, B_ENCODER_SW);
LiquidCrystal_I2C lcd(0x27,20,2);  // set the lcd address to 0x27 for a 16 chars and 2 line display

/* Menu structure...

    MainMenu
    |
    |--Run Application   // MenuAction
    |
    |--Enable            // MenuBoolValue
    |
    |--Speed             // MenuLongValue
    |
    |--Width             // MenuSmallFloatValue
    |
    |--Operation Mode    // MenuDropDownListValue
    |
    |--Colour            // MenuRotaryListValue
    |
    |--Configuration     // Menu
        |
        |--Brightness        // MenuRotaryListValue
        |
        |--Volume            // MenuLongValue
*/

bool enable = False;
long speed = 1000;
float width = 1.00
int operationMode = 0;
int colour = 0;

int brightness = 3;
long volume = 50;

MenuAction mmRun = MenuAction("Run Application", appFn, appInputFn)
MenuBoolValue mmEnable = MenuBoolValue("Enable", "Disable", "Enable", &enable);
MenuLongValue mmSpeed = MenuLongValue("Speed", "rpm", 0, 2000, 100, 10, &speed);
MenuSmallFloatValue mmWidth = MenuSmallFloatValue("Width", "mm", 0.001, 2.5, &width);
MenuDropDownListValue mmOperationMode = MenuDropDownListValue("Operation Mode", (const char *[]){"Automatic", "Manual", "Test", "Once (only)", nullptr}, &operationMode);
MenuRotaryListValue mmColour = MenuRotaryListValue("Colour", (const char *[]){"Red", "Green", "Blue", "Orange", "Purple", "Cyan", "Magenta", nullptr}, &colour);

MenuRotaryListValue cfgBrightness = MenuRotaryListValue("Volume", (const char *[]){"Minimum", "Dim", "Medium", "Bright", "Brightest", nullptr}, &brightness;
MenuLongValue cfgVolume = MenuLongValue("Volume", "%", 0, 100, 10, 1, &volume);

Menu cfgMnu = Menu(
    "Configuration",
    (BaseMenu *[]) {
        &cfgBrightness,
        &cfgVolume,
        nullptr
    }
);

Menu mm = Menu(
    "Main Menu"
    (BaseMenu *[]) {
mmRun,
mmEnable,
mmSpeed,
mmWidth = MenuSmallFloatValue("Width", "mm", 0.001, 2.5, &width);
mmOperationMode = MenuDropDownListValue("Operation Mode", (const char *[]){"Automatic", "Manual", "Test", "Once (only)", nullptr}, &operationMode);
mmColour = MenuRotaryListValue("Colour", (const char *[]){"Red", "Green", "Blue", "Orange", "Purple", "Cyan", "Magenta", nullptr}, &colour);
    }
);

void InitEncoders()
{
	aEncoder.setEncoderType( EncoderType::HAS_PULLUP );
	aEncoder.setBoundaries( 0, 1, false );
	aEncoder.begin();

	bEncoder.setEncoderType( EncoderType::HAS_PULLUP );
	bEncoder.setBoundaries( 0, 1, false );
	bEncoder.begin();
}

void appFn(BaseMenu *ownerMenu, ENCODER_SOURCE source, ENCODER_EVENT event, unsigned long value, BaseMenu *returningMenu)
{

}

void appInputFn(ENCODER_SOURCE source, ENCODER_EVENT event, unsigned long value, BaseMenu *owner)
{

}

void setup() {
    Serial.begin(115200);
	InitEncoders();
	Wire.begin(SDA_PIN,SCL_PIN);

	// Currently, the menu system only supports 1602 displays! (but you still need to specify the size!)

	BaseMenu::init(16, 2, &lcd, &aEncoder, &bEncoder);

	mainMenu.takeFocus();
}

void loop() {
}