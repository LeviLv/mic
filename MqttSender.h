#ifndef MQTT_SENDER_H
#define MQTT_SENDER_H

#include <PubSubClient.h>
#include <WiFi.h>

class MQTTSender
{
public:
    MQTTSender(const char *mqttServer, int mqttPort);
    bool connect();
    bool publish(const char *topic, const char *payload);
    void disconnect();

private:
    WiFiClient wifiClient;
    PubSubClient mqttClient;
    const char *ssid;
    const char *password;
    const char *mqttServer;
    int mqttPort;
};

#endif