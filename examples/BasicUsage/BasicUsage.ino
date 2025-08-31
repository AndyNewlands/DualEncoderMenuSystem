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

bool direction = false;
long speed = 1000;
float width = 1.00;
int operationMode = 0;
int colour = 0;

int brightness = 3;
long volume = 50;

// NOTE: ALL list values (lists of string pointers and lists or menu pointers) in DualEncoderMenuSystem
// MUST be terminated with a nullptr - failure to do this will result in an application crash!

void appFn(BaseMenu *ownerMenu, ENCODER_SOURCE source, ENCODER_EVENT event, unsigned long value, BaseMenu *returningMenu);
void appInputFn(ENCODER_SOURCE source, ENCODER_EVENT event, unsigned long value, BaseMenu *ownerMenu);
MenuAction mmRun = MenuAction("Run App.", appFn, appInputFn);

MenuBoolValue mmDirection = MenuBoolValue("Set Rotation", "C.W.", "C.C.W", &direction);
MenuLongValue mmSpeed = MenuLongValue("Set Speed", "rpm", 0, 2000, 100, 10, &speed);
MenuFloatValue mmWidth = MenuFloatValue("Set Width", "mm", 0.001, 2.5, 0.1, 0.005, &width);
MenuDropDownListValue mmOperationMode = MenuDropDownListValue("Select Mode", (const char *[]){"Automatic", "Manual", "Test", "Once (only)", nullptr}, &operationMode);
MenuRotaryListValue mmColour = MenuRotaryListValue("Colour", (const char *[]){"Set Red", "Set Green", "Set Blue", "Set Orange", "Set Purple", "Set Cyan", "Set Magenta", nullptr}, &colour);

MenuDropDownListValue cfgBrightness = MenuDropDownListValue("Set Brightness", (const char *[]){"Minimum", "Dim", "Medium", "Bright", "Brightest", nullptr}, &brightness);
MenuLongValue cfgVolume = MenuLongValue("Set Volume", "%", 0, 100, 10, 1, &volume);

Menu cfgMnu = Menu(
    "Configuration",
    (BaseMenu *[]) {
        &cfgBrightness,
        &cfgVolume,
        nullptr
    }
);

Menu mainMenu = Menu(
    "Main Menu",
    (BaseMenu *[]) {
        &mmRun,
        &mmDirection,
        &cfgMnu,
        &mmSpeed,
        &mmWidth,
        &mmOperationMode,
        &mmColour,
        nullptr
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
    Serial.println("appFn");
}

void appInputFn(ENCODER_SOURCE source, ENCODER_EVENT event, unsigned long value, BaseMenu *ownerMenu)
{
    Serial.println("appInputFn");
    if (source == ENCODER_SOURCE::A && event == ENCODER_EVENT::PRESSED)
        ownerMenu->returnFocus(source, event, value);
}

void setup() {
    Serial.begin(115200);
	InitEncoders();
	Wire.begin(SDA_PIN,SCL_PIN);

	// Currently, the menu system only supports 1602 displays! (but you still need to specify the size!)
	BaseMenu::begin(16, 2, &lcd, &aEncoder, &bEncoder);

	mainMenu.takeFocus();
}

void loop() {
}