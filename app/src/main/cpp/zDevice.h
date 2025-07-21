//
// Created by lxz on 2025/7/10.
//

#ifndef OVERT_ZDEVICE_H
#define OVERT_ZDEVICE_H


#include "config.h"

class zDevice {
private:
    zDevice();

    zDevice(const zDevice&) = delete;

    zDevice& operator=(const zDevice&) = delete;

    static zDevice* instance;

public:

    static zDevice* getInstance() {
        if (instance == nullptr) {
            instance = new zDevice();
        }
        return instance;
    }

    ~zDevice();

    static map<string, map<string, map<string, string>>> device_info;

    map<string, map<string, map<string, string>>>& get_device_info(){
        return device_info;
    };


};

#endif //OVERT_ZDEVICE_H
