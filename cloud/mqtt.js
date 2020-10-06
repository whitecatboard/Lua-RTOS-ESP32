/*
 * Whitecat Ecosystem, application function callbacks for whitecat MQTT
 * integration.
 * 
 * Copyright (C) 2015 - 2016
 * IBEROXARXA SERVICIOS INTEGRALES, S.L.
 *
 * Author: Jaume Olivé (jolive@iberoxarxa.com / jolive@whitecatboard.org)
 *
 * All rights reserved.
 *
 * Permission to use, copy, modify, and distribute this software
 * and its documentation for any purpose and without fee is hereby
 * granted, provided that the above copyright notice appear in all
 * copies and that both that the copyright notice and this
 * permission notice and warranty disclaimer appear in supporting
 * documentation, and that the name of the author not be used in
 * advertising or publicity pertaining to distribution of the
 * software without specific, written prior permission.
 *
 * The author disclaim all warranties with regard to this
 * software, including all implied warranties of merchantability
 * and fitness.  In no event shall the author be liable for any
 * special, indirect or consequential damages or any damages
 * whatsoever resulting from loss of use, data or profits, whether
 * in an action of contract, negligence or other tortious action,
 * arising out of or in connection with the use or performance of
 * this software.
 */

require("./settings.js")
require("./pack.js")
require("./app.js")

var mqtt = require('mqtt')

var MQTTClient = null;

MQTTConnect = function() {
	if (typeof MQTTSettings != "undefined") {
		// Sanity checks
		if (typeof MQTTSettings.broker == "undefined") {
			console.log("MQTTSettings, missing broker");
			return;
		}

		if (typeof MQTTSettings.user == "undefined") {
			console.log("MQTTSettings, missing user");
			return;
		}

		if (typeof MQTTSettings.password == "undefined") {
			console.log("MQTTSettings, missing password");
			return;
		}

		if (typeof MQTTSettings.port == "undefined") {
			console.log("MQTTSettings, missing port");
			return;
		}
		
		// Connect
		console.log("Connecting to " + MQTTSettings.broker);
	
		MQTTClient  = mqtt.connect(MQTTSettings.broker, {
			"username": MQTTSettings.user,
			"password": MQTTSettings.password,
			"port": MQTTSettings.port
		});

		MQTTClient.on('connect', function () {
			console.log("connected to " + MQTTSettings.broker);
			console.log()
		
			MQTTClient.subscribe("/ide/+/+");
		});

		MQTTClient.on('message', function (topic, message) {
			// Call to callback
			if (typeof MQTTSettings.callback == "function") {
				MQTTSettings.callback(topic, message.toString());
			}
		});	
	} else {
		console.log("No MQTTSettings defined, skipping");
	}
}

MQTTPublish = function(topic, message) {
	MQTTClient.publish(topic, message);
}