//
// Created by lxz on 2025/7/10.
//

#ifndef OVERT_ZDEVICE_H
#define OVERT_ZDEVICE_H

#include <shared_mutex>
#include "config.h"
#include "zLog.h"


class zDevice {
private:
    zDevice();

    zDevice(const zDevice&) = delete;

    zDevice& operator=(const zDevice&) = delete;

    static zDevice* instance;

    static map<string, map<string, map<string, string>>> device_info;

    mutable std::shared_mutex device_info_mtx_;

public:

    static zDevice* getInstance() {
        if (instance == nullptr) {
            instance = new zDevice();
        }
        return instance;
    }

    ~zDevice();

    const map<string, map<string, map<string, string>>>& get_device_info() const;

    void update_device_info(const string& key, const map<string, map<string, string>>& value);

};

#endif //OVERT_ZDEVICE_H
