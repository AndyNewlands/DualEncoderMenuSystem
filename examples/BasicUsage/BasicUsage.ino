#include <DualEncoderMenuSystem.h>

DualEncoderMenuSystem menuSystem;

const char* menuItems[] = {
    "Item 1",
    "Item 2",
    "Item 3",
    "Item 4"
};

void setup() {
    Serial.begin(9600);
    menuSystem.begin();
    menuSystem.setMenu(menuItems, sizeof(menuItems) / sizeof(menuItems[0]));
}

void loop() {
    menuSystem.update();
    int selectedItem = menuSystem.getSelectedItem();
    
    Serial.print("Selected Item: ");
    Serial.println(menuItems[selectedItem]);
    
    delay(500); // Adjust delay as needed for readability
}