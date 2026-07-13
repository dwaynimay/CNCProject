require('dotenv').config();
module.exports = {
    host:     process.env.MQTT_HOST     || 'localhost',
    port:     process.env.MQTT_PORT     || 1883,
    topicSub:      process.env.MQTT_TOPIC_SUB       || 'cnc/+/telemetry',
    topicCmd:      process.env.MQTT_TOPIC_CMD       || 'cnc/%s/command',
    topicSelfTest: process.env.MQTT_TOPIC_SELFTEST  || 'cnc/+/selftest_result',
};
