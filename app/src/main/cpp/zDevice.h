//
// Created by lxz on 2025/7/10.
//

#ifndef OVERT_ZDEVICE_H
#define OVERT_ZDEVICE_H

#include <map>
#include <string>

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

    static std::map<std::string, std::map<std::string, std::map<std::string, std::string>>> device_info;

    std::map<std::string, std::map<std::string, std::map<std::string, std::string>>>& get_device_info(){
        return device_info;
    };


};

#endif //OVERT_ZDEVICE_H
