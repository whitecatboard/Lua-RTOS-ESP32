require("./app.js")

// TTN credentials
TTNSettings = {
	"broker": "mqtt://eu.thethings.network",
	"user": "xxx",
	"password": "xxx",
	"port": 1883,
	"callback": TTNCallback
}

// MQTT credentials
MQTTSettings = {
	"broker": "mqtts://mqtt.whitecatboard.org",
	"user": "ide",
	"password": "pin8miente",
	"port": 8883,
	"callback": MQTTCallback
}
