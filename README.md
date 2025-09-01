# DualEncoderMenuSystem
A simple menu system for 1602 LCDs, using I2C, operated by TWO rotary encoders

## Background
I wrote this for TWO rotary encoders because my coil-winder project called for two encoders: one to control the bobbin stepper motor and the other to control the wire feeder stepper motor.
Using TWO encoders adds the opportunity to support coarse and fine value adjustments (in my project, for example, to more easily set the number coil windings and the wire diameter).

## Compatibility
This was written for use with an ESP32 Dev board.  I have not tried with Arduino but I see no reason why it should not not work.

This is the hardware I have tested with:
* 1 x ESP32 WROOM DevKit
* 1 x HD44780 16x2 LCD Display, driven by I2C via a PCF8574T (to save IO pins)
* 2 x KY-040 Rotary Encoder Modules

## Current Limitations
Only 16 x 2 LCD displays are currently supported. With a little more work, the code could be adapted to support displays with dimension other than 16 x 2 (but I only needed this to work with a 1602 display, so that's as far as a went)

## Custom Characters
**IMPORTANT**: The Menu System defines custom characters in the following LCD custom character slots: 1, 2, 3 & 4 - please use other slots for for own custom characters, accept that writing your own characters to these slots will impact Menu item appearance, or change the relevant library code in MenuSystem::begin()

## Menu System Classes
* `MenuSystem` - base class from which all other menu item classes are derived
* `Menu` - contains a list of one or more MenuSystem-derived objects (including *Menu*) and allows each to be interacted with via the rotary encoders
* `MenuBoolValue` - operates on a boolean value. Has a name and true/false option strings. For example: "Are you sure?" -> "Yes" / "No"
* `MenuLongValue` - operates on a long value. Has a name, minimum and maximum values, pluse a step size for each rotary encoder, to enable coarse/fine value adjustment (these can be set to the same size step, if preferred)
* `MenuFloatValue` - operates on a float value. Similar to *MenuLongValue*, exept this operates on a float.
* `MenuDropDownListValue` - operates on an integer value, which reflects the zero-based index of a user-selected item from a list of strings. Has a name and a list of options. For example: "Set Speed" -> "Slow", "Medium", "Fast".
* `MenuRotaryListValue` - similar to `MenuDropDownListValue` but does not operate in its screen.  Instead, the selected list item is changed each time the user clicks one of the encoders, without leaving the owner `Menu`.
* `MenuAction` - executes and developer-defined function and sends all encoder input to a realted developer-defined function, until the developer-defined code return input focus to the `Menu` from which the Action was invoked.

**NOTE 1**: With the exception of `MenuRotaryListValue`, ALL classes which operate on a value, set up their own display and value editor, to allow the user to alter the value (within the parameters specified by the developer).  Once editing is complete, the user can return to the parent menu by clicking one of the rotary encoders.

**NOTE 2**: The `MenuAction` merely invokes the provided developer-define function, when clicked.  Encoder input is then received by the additionally developer-provider input function.  The developer is entirely responsible from display and input handing until they want to return focus to the previous menuy (which will redraw itself).  The action's function may legitimately invoke its own menu items (for example, "Please Select" -> "Continue", "Pause" "Exit"), to interact with the users while "running" (the Action can kepp input focus, even when its function completes - this allows the main loop() function to continue being called). When those menus quit (return focus to the Action), the Action's main function will be called again, and will be notified of the menu item which returned focus.  See the example app for a better view of how that works.

**NOTE 3**: All values operated on by menu objects, are passed to those objects as pointers.  Be aware that the same value pointer, passed to more than one menu object, will result in multiple menu items being able to change that value (which may or may not be useful).

**NOTE 4**: Menu items are passed to the `Menu` constructor as a list of pointers (`MenuSystem *`). Therefore, as with values (in NOTE 3), a single menu item may appear in multiple `Menu` obejcts (which may or may not be useful).

**NOTE 5**: `Menu`s can be nested.  This allows a menu hierachry to be implement.

**IMPORTANT** All lists (`char **` and `MenuSystem **`) must be null-terminated, so that the library knows the number of list-items - the alternative would be to require a list-length to be explicitly specified (I find null termination easier, when it comes to altering lists during development).  If your application starts exhibiting chaotic/unstable/random behaviour, please check ALL your Menu System lists are null terminated (if they are not, the library code will try to operate beyond the end of your list, with unpredictable results!).

**IMPORTANT** Do NOT create `MenuSystem` object in temporary storage on the stack (for example, in the setup() or loop() functions) as this will crash your application.  This is because these will be destroyed as soon as the function, in which the objects were created, exits BUT the encoder input focus will still be routing to the now-destoyed object (your app will likely crash, when you turn or press an encoder).  Instead, create ALL Menu System items as globals in your sketches.

## Methods
* `MenuSystem::begin()` - This must be called BEFORE using any menus.  This "connects" the Menu System with your rotary encoders and LCD.
* `takeFocus()` - Sends encoder input to the menu (which will also draw itself).
* `returnFocus()` - Menus call this on each other, but it should also be called, which an Action want to complete and return the use to the menu system.

Method-wise, there really isn't anything else to be aware of BUT to use this effectively, you need to understand how the Menu System should be structured and, most importantly, understand how the `MenuAction` works.  Reading/running/experimenting-with the provided example is the best way to achieve that.

## Example
The provided example show an example of using each of the _Menu System_ classes, including nesting menus and invokving an Action (using `MenuAction`) and how maintaining state imformation allows your Action to continue "running" while still allowing the loop() function to be regularly called (I.E. without "blocking")

The example code shows how to implement the following simple menu structure...

'/* Menu structure...

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
*/'

## Issues / Contributions

If you have any issues with this library, or have a feature request, log it on GitHub.

If you wish to contribute, please feel free to make a pull-request.

