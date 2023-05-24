#include "MqttSender.h"

MQTTSender::MQTTSender(const char *mqttServer, int mqttPort)
    : mqttServer(mqttServer), mqttPort(mqttPort), mqttClient(wifiClient)
{
}

bool MQTTSender::connect()
{
    mqttClient.setServer(mqttServer, mqttPort);

    return mqttClient.connect("ESP32Client");
}

bool MQTTSender::publish(const char *topic, const char *payload)
{
    return mqttClient.publish(topic, payload);
}

void MQTTSender::disconnect()
{
    mqttClient.disconnect();
}