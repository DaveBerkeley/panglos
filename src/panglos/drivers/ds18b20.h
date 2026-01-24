
#pragma once

namespace panglos {

class TemperatureSensor;
class OneWire;
class Device;

TemperatureSensor *create_ds18b20(OneWire *onewire, uint64_t addr);

bool init_ds18b20(Device *dev, void *arg);

}   //  panglos

//  FIN
