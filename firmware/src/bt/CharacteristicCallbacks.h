#ifndef CHARACTERISTIC_CALLBACKS_H
#define CHARACTERISTIC_CALLBACKS_H

#include <BLECharacteristic.h>

class CharacteristicCallbacks final : public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) override;
};

#endif