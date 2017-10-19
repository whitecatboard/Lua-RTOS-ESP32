/*
 * Whitecat Ecosystem, application function callbacks for TTN
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

var TTNClient = null;

TTNConnect = function() {
	if (typeof TTNSettings != "undefined") {
		// Sanity checks
		if (typeof TTNSettings.broker == "undefined") {
			console.log("TTNSettings, missing broker");
			return;
		}

		if (typeof TTNSettings.user == "undefined") {
			console.log("TTNSettings, missing user");
			return;
		}

		if (typeof TTNSettings.password == "undefined") {
			console.log("TTNSettings, missing password");
			return;
		}

		if (typeof TTNSettings.port == "undefined") {
			console.log("TTNSettings, missing port");
			return;
		}
		
		// Connect
		console.log("Connecting to " + TTNSettings.broker);
	
		TTNClient  = mqtt.connect(TTNSettings.broker, {
			"username": TTNSettings.user,
			"password": TTNSettings.password,
			"port": TTNSettings.port
		});

		TTNClient.on('connect', function () {
			console.log("connected to " + MQTTSettings.broker);
			console.log()
		
			TTNClient.subscribe("+/devices/+/up");
		});

		TTNClient.on('message', function (topic, message) {
			// Convert message to a JSON object
			message = JSON.parse(message.toString());
	
			// Get the relevant parts of the message
	
			// Port
			var port = message.port;
	
			// Payload
			// payload_raw is encoded in base64, so first decode
			// then unpack
			var payload = new Buffer(message.payload_raw, 'base64');
	
			payload = unpack(payload);
	
			// Remove irrelevant parts of message
			delete message.metadata;
			delete message.payload_raw;
	
			// Add decoded payload to message
			message.payload = payload

			// Call to callback
			if (typeof TTNSettings.callback == "function") {
				TTNSettings.callback(message);
			}
		});	
	} else {
		console.log("No TTNSettings defined, skipping");
	}
}

TTNSend = function(device, port, payload) {
	TTNClient.publish(TTNSettings.user + "/devices/" + device + "/down", JSON.stringify({
		"port": port,
		"payload_raw": payload.toString('base64')
	}));
};
