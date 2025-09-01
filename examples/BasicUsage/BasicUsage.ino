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

enum MODE {
    NORMAL,
    APP_RUNNING,
    APP_PAUSED,
    APP_MENU
};

// Used to control what happens in main loop
MODE mode = MODE::NORMAL;

// Must instantiate encoders and LCD
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

// Declare forward references
void appFn(MenuSystem *ownerMenu, ENCODER_SOURCE source, ENCODER_EVENT event, unsigned long value, MenuSystem *returningMenu);
void appInputFn(ENCODER_SOURCE source, ENCODER_EVENT event, unsigned long value, MenuSystem *ownerMenu);

// Vairables managed from "Main Menu"
bool direction = false;
long speed = 1000;
float width = 1.00;
int operationMode = 0;
int colour = 0;

// Vairables managed from "Config menu"
int brightness = 2;
long volume = 50;

/********************************************************************************************************/
// NOTE: ALL list values (lists of string pointers and lists or menu pointers) in DualEncoderMenuSystem
// MUST be terminated with a nullptr - failure to do this will result in an application crash!
/********************************************************************************************************/

// Construct menu items ahead of the menu in which they appear (in the case, the "Main menu")
MenuAction mmRun = MenuAction("Run App.", appFn, appInputFn);
MenuBoolValue mmDirection = MenuBoolValue("Set Rotation", "C.W.", "C.C.W", &direction);
MenuLongValue mmSpeed = MenuLongValue("Set Speed", "rpm", 0, 2000, 100, 10, &speed);
// NOTE: The coarse and fine step settings for MenuFloatValue - encoder A makes coarse changes
// to the value, while encoder B makes finer changes (or) they can be set to the same value)
MenuFloatValue mmWidth = MenuFloatValue("Set Width", "mm", 0.001, 2.5, 0.1, 0.005, &width);
MenuDropDownListValue mmOperationMode = MenuDropDownListValue(
    "Select Mode",
    (const char *[]){
        "Automatic",
        "Manual",
        "Test",
        "Once (only)",
        0  // <-- Do NOT forget to null-terminate!
    },
    &operationMode);
MenuRotaryListValue mmColour = MenuRotaryListValue(
    "Colour",
    (const char *[]){
        "Set Red",
        "Set Green",
        "Set Blue",
        "Set Orange",
        "Set Purple",
        "Set Cyan",
        "Set Magenta",
        0  // <-- Do NOT forget to null-terminate!
    },
    &colour);

// Construct menu items ahead of the menu in which they appear (in the case, the "Configuration Menu")
MenuLongValue cfgVolume = MenuLongValue("Set Volume", "%", 0, 100, 10, 1, &volume);                   // Do NOT forget to null terminate lists >|<
MenuDropDownListValue cfgBrightness = MenuDropDownListValue("Set Brightness", (const char *[]){"Minimum", "Dim", "Medium", "Bright", "Maximum", 0}, &brightness);

// Constuct the sub menu first, so it can be incorporated into it parent ("Main Menu")
Menu cfgMnu = Menu(
    "Configuration",
    (MenuSystem *[]) {
        &cfgBrightness,
        &cfgVolume,
        &mmWidth, // <-- NOTE: A single menu item can be used in more than one place - mmWidth appears in the both configuration and main menus
        0  // <-- Do NOT forget to null-terminate!
    }
);

// Now we can add the items we consturcted, above, into the main menu
Menu mainMenu = Menu(
    "Main Menu",
    (MenuSystem *[]) {
        &mmRun,
        &mmDirection,
        &cfgMnu,
        &mmSpeed,
        &mmWidth,
        &mmOperationMode,
        &mmColour,
        0 // <-- Do NOT forget to null-terminate!
    }
);

// Initialise our TWO encoders
void InitEncoders()
{
	aEncoder.setEncoderType( EncoderType::HAS_PULLUP ); // Set this IF your encoder has its own built in pullup resistor
	aEncoder.setBoundaries( 0, 1, false );
	aEncoder.begin();

	bEncoder.setEncoderType( EncoderType::HAS_PULLUP ); // Set this IF your encoder has its own built in pullup resistor
	bEncoder.setBoundaries( 0, 1, false );
	bEncoder.begin();
}

// Custom LCD charcaters (see note, further down)

byte custChar1[] = {
    0b00100,
    0b11111,
    0b11011,
    0b11011,
    0b11011,
    0b11011,
    0b11011,
    0b00000};

byte custChar2[] = {
    0b00000,
    0b00000,
    0b00000,
    0b11111,
    0b11011,
    0b11011,
    0b11011,
    0b00000};

byte custChar3[] = {
    0b00000,
    0b00000,
    0b00000,
    0b00000,
    0b11111,
    0b11111,
    0b11011,
    0b00000};

// This menu item is only ever called from the app function
int appOptionValue = 0;
MenuDropDownListValue appOptions = MenuDropDownListValue("App. Options", (const char *[]){"Continue", "Pause", "Exit", 0}, &appOptionValue);

char appData[17];
bool bRedraw = false;
long lastAppAnimationTime = 0;

// This is called from the menu to which this fn was passed (when it's clicked with one of the encoders)
void appFn(MenuSystem *ownerMenu, ENCODER_SOURCE source, ENCODER_EVENT event, unsigned long value, MenuSystem *returningMenu)
{
    // When the owner menu invokes this function, ownerMenu will point to that menu

    // If this 'app' invokes its own menu(s), this function will be called again,
    // and the returningMenu will point to the invoked menu when it exits
    // That allows this function to know which menu just exited and, thus,
    // which value may have been updated

    if (returningMenu == nullptr) {
        // Initial invokation
        mode = MODE::APP_RUNNING;
        appOptionValue = 0; // Always set the value to "Continue"
        bRedraw = true;
    } else {
        if (returningMenu == &appOptions) {
            switch(appOptionValue) {
                case 0:
                    // User clicked "Continue" (carry on with our animation)
                    mode = MODE::APP_RUNNING;
                    bRedraw = true;
                    break;
                case 1:
                    // User clicked "Pause" (Display the 'app' output, but don't do any more animation)
                    mode = MODE::APP_PAUSED;
                    bRedraw = true;
                    break;
                case 2:
                    // User clicked "Exit" (end the 'app' and return to the parent/owner menu)
                    mode = MODE::NORMAL;
                    ownerMenu->returnFocus(source, event, value);
                    break;
            }
        }
    }
}

// This function is passed to the MenuAction object constructor, and will be call when encoder input
// is received and the action menu has been selected
void appInputFn(ENCODER_SOURCE source, ENCODER_EVENT event, unsigned long value, MenuSystem *ownerMenu)
{
    if (event == ENCODER_EVENT::PRESSED) {
        // Activate app menu when either decoder is pressed
        mode = MODE::APP_MENU; // Ensure the app doesn't draw over the menu (see)
        appOptions.takeFocus();
    }
}

void appReDraw()
{
    // Completely redraw the app output
    // (for example, after a menu was displayed)
    lcd.clear();
    lcd.setCursor(0, 0);
    if (mode == MODE::APP_RUNNING)
        lcd.print("Running...");
    else if (mode == MODE::APP_PAUSED)
        lcd.print("Paused");
    appDrawData();
}

void appDrawData()
{
    // Display app data on bottom row
    lcd.setCursor(0, 1);
    lcd.print(appData);
}

void appAnimate()
{
    char c;
    // THIS simply animates the display to represent  some other app doing some "work"
    // (you'll write your own version of )
    if (bRedraw && (mode == MODE::APP_PAUSED || MODE::APP_RUNNING)) {
        appReDraw();
        bRedraw = false;
    }

    if (mode != APP_RUNNING)
        return; // No updates unless the app is running (and not just paused)
 
    long now = millis();
    if (now - lastAppAnimationTime < 300)
         return;
    // Update (animate the appData)
    for (int i = 0; i < 16; i++) {
        c = random(4, 8);
        if (c == 4)
            c = 0x20;
        appData[i] = c;
    }
    lastAppAnimationTime = now;

    // Display the updated data
    appDrawData();
}

void setup() {
    // In case we want to get some serial data
    Serial.begin(115200);
    // Musr initilise the TWO encoders
	InitEncoders();
    // Must call this (or no LCD output will be generated)
	Wire.begin(SDA_PIN,SCL_PIN);

    // NOTE: The menu system uses custom characters 1,2 & 3
    // These are used as part of the app animation
    lcd.createChar(5, custChar1);
    lcd.createChar(6, custChar2);
    lcd.createChar(7, custChar3);

	// Currently, the menu system only supports 1602 displays! (but you still need to specify the size!)
	MenuSystem::begin(16, 2, &lcd, &aEncoder, &bEncoder);

    // Start the main menu
	mainMenu.takeFocus();
}

void loop() {
    appAnimate();
}