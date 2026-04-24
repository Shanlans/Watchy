#include <Arduino.h>
#line 1 "E:\\CP\\Watchy\\firmware\\arduino\\Watchy_7SEG\\Watchy_7SEG.ino"
#include "Watchy_7_SEG.h"
#include "settings.h"

Watchy7SEG watchy(settings);

#line 6 "E:\\CP\\Watchy\\firmware\\arduino\\Watchy_7SEG\\Watchy_7SEG.ino"
void setup();
#line 10 "E:\\CP\\Watchy\\firmware\\arduino\\Watchy_7SEG\\Watchy_7SEG.ino"
void loop();
#line 6 "E:\\CP\\Watchy\\firmware\\arduino\\Watchy_7SEG\\Watchy_7SEG.ino"
void setup(){
  watchy.init();
}

void loop(){}




