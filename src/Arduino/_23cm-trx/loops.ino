/*
 * 
 */
 byte loopVfo() {
  Serial.println("--- Loop: VFO ---");
  createSMeterCharacters();
  readMemory(VFO_MEMORY_LOCATION);

  lcd.clear();
  setRxFreq(rxFreqHz);
  lcd.setCursor(13,0); lcd.print("VFO");    // Indicate unsaved changes

  // Write to EEPROM every 10 seconds;
  const int writeEvery = 10000;
  unsigned long nextEpromWrite = millis() + writeEvery;
  boolean unsavedChanges = false;

  while (1) {
    updateSmeterDisplay();
    
    /* Handle turning of the rotary encoder */
    long up = getRotaryTurn() * rasterHz;
    if (up != 0) {
      nextEpromWrite = millis() + writeEvery; // Hold off writes
      lcd.setCursor(15,0); lcd.print("0");    // Indicate unsaved changes
      unsavedChanges = true;
      rxFreqHz = constrain(rxFreqHz + up, minTxFreq, maxTxFreq);
      setRxFreq(rxFreqHz);
    }
  
    int push = getRotaryPush();
    if (push == 2) {
      /* Switch to Menu if long pushed */
      writeMemory(VFO_MEMORY_LOCATION);
      return LOOP_VFO_MENU;
    } else if (push == 1) {
      /* Switch to memory when short pushed */
      writeMemory(VFO_MEMORY_LOCATION);
      selectedMemory = 0;
      writeGlobalSettings();
      return LOOP_MEMORY;
    }
  
    /* Handle PTT */
    if (isPTTPressed()) { transmit(); }

    /* Write changes to EEPROM */
    if (unsavedChanges && millis() > nextEpromWrite) {
      writeMemory(VFO_MEMORY_LOCATION);
      lcd.setCursor(15,0); lcd.print("O");
      unsavedChanges = false;
    }      
  }
  return LOOP_VFO;
}


byte loopMemory() {
  Serial.println("--- Loop: Memory Mode ---");
  createSMeterCharacters();
  selectedMemory = lastSelectedMemory;
  
  readMemory(selectedMemory);
  lcd.clear();
  setRxFreq(rxFreqHz);
  lcd.setCursor(14,0); lcd.print("M"); lcd.print(selectedMemory);

  while (1) {
    updateSmeterDisplay();
    
    int push = getRotaryPush();
    if (push == 2) { 
      return LOOP_MEMORY_MENU; 
    } else if (push == 1) {
      /* Switch to VFO when short pushed */
      writeMemory(selectedMemory);
      selectedMemory = VFO_MEMORY_LOCATION;
      writeGlobalSettings();
      return LOOP_SPECTRUM;
    }

    int up = getRotaryTurn();
    if (up != 0) {
      selectedMemory = constrain(selectedMemory + up, 0, VFO_MEMORY_LOCATION-1);
      lastSelectedMemory = selectedMemory;

      readMemory(selectedMemory);
      setRxFreq(rxFreqHz);
      lcd.setCursor(14,0); lcd.print("M"); lcd.print(selectedMemory);
      writeGlobalSettings();
    }

    /* Handle PTT */
    if (isPTTPressed()) { transmit(); }
  }
}


byte loopVfoMenu() {

  Serial.println("--- Loop: VFO Menu ---");
  int menuitem = 0;
  boolean exit = false;

  while(!exit) {
    byte max_menu_items = 5;

    
    Serial.print("Menu "); Serial.println(menuitem);
    switch(menuitem) {
      // --------------------------------------------------- Squelch menu
      case 0 : {
        int turn = selectInt("Squelch level", " ", squelchlevel , 0, 9, 1);
        if (turn == 0) { exit = true; }
        else { menuitem = constrain(menuitem + turn, 0, max_menu_items); }
      } break;
      // ------------------------------------------------ Subaudio/CTCSS menu
      case 1 : 
        lcd.clear();
        lcd.setCursor(0,0); lcd.print("> Subaudio tone");
        lcd.setCursor(2,1); printCTCSS();

        while(!exit && menuitem == 1) {
          readRSSI();                  // Mute/unmute audio based on squelch.
          setMenuItem(menuitem, max_menu_items);
          byte push = getRotaryPush();
          if (push == 2) { exit = true; } // long push, exit
          if (push == 1) {
            lcd.setCursor(0,0); lcd.print(" ");
            lcd.setCursor(0,1); lcd.print(">"); // Rotary now selects tone
            while (0 == getRotaryPush()) { // until rotary is pushed again
              readRSSI();                  // Mute/unmute audio based on squelch.
              int turn = getRotaryTurn();
              if (turn != 0) {
                subAudioIndex = constrain(subAudioIndex + turn, -1, 38); // See subaudio.ino
                lcd.setCursor(2,1); printCTCSS();
              }
            }
            exit = true;
          }
        }
      break;
      // ------------------------------------------------ Repeater Shift.
      case 2: 
        lcd.clear();
        lcd.setCursor(0,0); lcd.print("> Repeater shift");
        lcd.setCursor(2,1); printRepeaterShift();

        while(!exit && menuitem == 2) {
          readRSSI();                  // Mute/unmute audio based on squelch.
          setMenuItem(menuitem, max_menu_items);
          byte push = getRotaryPush();
          if (push == 2) { exit = true; } // long push, exit
          if (push == 1) {
            lcd.setCursor(0,0); lcd.print(" ");
            lcd.setCursor(0,1); lcd.print(">"); // Rotary now selects Shift
            while (0 == getRotaryPush()) { // until rotary is pushed again
              readRSSI();                  // Mute/unmute audio based on squelch.
              int turn = getRotaryTurn();
              if (turn != 0) {
                repeaterShiftIndex = constrain(repeaterShiftIndex + turn, 0, 4); // See PLL.ino
                lcd.setCursor(2,1); printRepeaterShift();
              }
            }
            exit = true;
          }
        }
      break;
      // ------------------------------------------------ Memory Write.
      case 3: 
        lcd.clear();
        lcd.setCursor(0,0); lcd.print("> Memory Write ");

        while(!exit && menuitem == 3) {
          readRSSI();                  // Mute/unmute audio based on squelch.
          setMenuItem(menuitem, max_menu_items);
          byte push = getRotaryPush();
          if (push == 2) { exit = true; } // long push, exit
          if (push == 1) {
            lcd.setCursor(0,0); lcd.print(" ");
            lcd.setCursor(0,1); lcd.print("> Cancel"); // Rotary now selects memory

            int writeToMemory = -1;
            while (0 == getRotaryPush()) { // until rotary is pushed again
              readRSSI();                  // Mute/unmute audio based on squelch.
              int turn = getRotaryTurn();
              if (turn != 0) {
                writeToMemory = constrain(writeToMemory + turn, -1, VFO_MEMORY_LOCATION - 1);
                lcd.setCursor(2,1); 
                if (writeToMemory == -1) {
                   lcd.print("Cancel    ");
                } else {
                   lcd.print("VFO to M");
                   lcd.print((int)writeToMemory);
                }
              }
            }
            if (writeToMemory >= 0 && writeToMemory < VFO_MEMORY_LOCATION) {
              writeMemory(writeToMemory);
            }
            exit = true;
          }
        }
      break;
      // ------------------------------------------------ Backlight brightness
      case 4: {
        int turn = selectInt("Backlight", "  ", lcdBacklightBrightness , 5, 255, 10);
        if (turn == 0) { exit = true; }
        else { menuitem = constrain(menuitem + turn, 0, max_menu_items); }       
      }
      break;
      // ------------------------------------------------ TCXO Frequency
      case 5: {
        int tcxoInt = tcxoRefHz / 1000;
        int turn = selectInt("TCXO Frequency", " kHz", tcxoInt , minTcxoRefHz/1000, maxTcxoRefHz/1000, 100);
        if (turn == 0) { 
          tcxoRefHz = 1000 * long(tcxoInt);
          setupPLL();
          exit = true; 
        } else { 
          menuitem = constrain(menuitem + turn, 0, max_menu_items);
        }
      }
      break;
      // ------------------------------------------------ Out of bounds.
      default :
        Serial.println("Menu item out of bounds.");
        setMenuItem(menuitem, max_menu_items);
    }
    // Exit menu, write all changes to EEPROM
    writeGlobalSettings();
    writeMemory(VFO_MEMORY_LOCATION);
  }
  return LOOP_VFO;
}

void setMenuItem(int& menuitem, byte size) {
  menuitem = menuitem + getRotaryTurn();
  menuitem = constrain(menuitem, 0, size);  
}

byte loopMemoryMenu() {
  Serial.println("--- Loop: Memory Menu ---");
  int menuitem = 0;
  boolean exit = false;

  while(!exit) {
    byte max_menu_items = 1;
    Serial.print("Menu "); Serial.println(menuitem);
    switch(menuitem) {
      // --------------------------------------------------- Squelch menu
      case 0 : {
        int turn = selectInt("Squelch level", " ", squelchlevel , 0, 9, 1);
        if (turn == 0) { exit = true; }
        else { menuitem = constrain(menuitem + turn, 0, max_menu_items); }
      } break;
      case 1 : {
        lcd.clear();
        lcd.setCursor(0,0); lcd.print("> Scan ");

        while(!exit && menuitem == 1) {
          readRSSI();                  // Mute/unmute audio based on squelch.
          setMenuItem(menuitem, max_menu_items);
          byte push = getRotaryPush();
          if (push == 2) { exit = true; } // long push, exit
          if (push == 1) {
            lcd.setCursor(0,0); lcd.print(" ");
            lcd.setCursor(0,1); lcd.print(">"); // Rotary now selects scan mode
                                                  
            String scanmode[] = {"Stop at signal", "Wait QSO end  "}; 
            int index = 0;
            lcd.setCursor(2,1); lcd.print(scanmode[index]);
            while (0 == getRotaryPush()) { // until rotary is pushed again
              readRSSI();                  // Mute/unmute audio based on squelch.
              int turn = getRotaryTurn();
              if (turn != 0) {
                index = constrain(index + turn, 0, 1);
                lcd.setCursor(2,1); lcd.print(scanmode[index]);
              }
            }

            while (true) {
              /* Scan for signal*/
              lcd.setCursor(0,1); lcd.print("Scanning...     "); 
              do {
                selectedMemory += 1;
                if (selectedMemory == VFO_MEMORY_LOCATION) { selectedMemory = 0; }
                readMemory(selectedMemory);
                setRxFreq(rxFreqHz);
        
                lastSelectedMemory = selectedMemory;
                lcd.setCursor(14,0); lcd.print("M"); lcd.print(selectedMemory);
                delay(50); // Wait for S-meter to stabilize
                
                if (getRotaryPush()) { return LOOP_MEMORY; }
              } while (readRSSI() < (squelchlevel*100));
  
              // scan mode 0, exit
              if (index=0) { return LOOP_MEMORY; }
  
              // scan mode 1, wait for silence, repeat.
              lcd.setCursor(0,1); lcd.print("Waiting...  "); 
              unsigned long continueAt = millis() + 20000;
              do {
                if (getRotaryPush() || isPTTPressed()) { return LOOP_MEMORY; }
                if (readRSSI() >= (squelchlevel*100)) { continueAt = millis() + 20000; }
              } while (millis() < continueAt);
            }
          }
        }
      }
      default :
        Serial.println("Menu item out of bounds.");
        setMenuItem(menuitem, max_menu_items);
    }
  }
  // Exit menu, write all changes to EEPROM
  writeGlobalSettings();
  return LOOP_MEMORY;
}




