//
// Created by lxz on 2025/8/2.
//

#ifndef OVERT_ZBROADCAST_H
#define OVERT_ZBROADCAST_H

#include <shared_mutex>
#include "zStd.h"
#include "zLog.h"

struct ThreadArgs{
    int port;
    string msg;
    void (*on_receive)(const char* ip, const char* msg);
};

class zBroadCast {
private:
    zBroadCast();

    zBroadCast(const zBroadCast&) = delete;

    zBroadCast& operator=(const zBroadCast&) = delete;

    static zBroadCast* instance;

    pthread_t local_ip_monitor_tid = 0;
    pthread_t sender_tid = 0;
    pthread_t listener_tid = 0;

    ThreadArgs sender_thread_args = {};
    ThreadArgs listener_thread_args = {};
    ThreadArgs stop_thread_args = {};

    mutable std::shared_mutex sender_thread_args_mutex;
    mutable std::shared_mutex listener_thread_args_mutex;
    mutable std::shared_mutex stop_thread_args_mutex;

    mutable std::shared_mutex local_ip_mutex;
    mutable std::shared_mutex local_ip_c_mutex;

    string local_ip;
    string local_ip_c;

    int port;
    string msg;
    void (*on_receive)(const char* ip, const char* msg);




public:

    ThreadArgs& get_listener_thread_args(){
       return listener_thread_args;
    }



    static zBroadCast* getInstance();

    ~zBroadCast();

    void start_local_ip_monitor();
    string get_local_ip();
    void set_local_ip(string local_ip);
    string get_local_ip_c();
    void set_local_ip_c(string local_ip_c);





    void send_udp_broadcast();
    void send_udp_broadcast(int port, string message);
    void start_udp_broadcast_sender();
    void start_udp_broadcast_sender(int port, string msg);
    void set_sender_thread_args(int port, string msg, void (*on_receive)(const char* ip, const char* msg));

    void listen_udp_broadcast();
    void listen_udp_broadcast(int port, void (*on_receive)(const char* ip, const char* msg));
    void start_udp_broadcast_listener();
    void start_udp_broadcast_listener(int port, void (*on_receive)(const char* ip, const char* msg));
    void restart_udp_broadcast_listener();
    void set_listener_thread_args(int port, string msg, void (*on_receive)(const char* ip, const char* msg));
    void set_stop_thread_args(int port, string msg, void (*on_receive)(const char* ip, const char* msg));

};


#endif //OVERT_ZBROADCAST_H
