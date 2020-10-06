/*
 * Whitecat Ecosystem, application integration
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

require("./mqtt.js")
require("./ttn.js")
require("./pack.js")

var exec = require('child_process').exec;

function getTTNDeviceInfo(node, callback) {
	child = exec("ttnctl devices info " + node, function (error, stdout, stderr) {
		var AppEUI  = "";
		var DevEUI  = "";
		var DevAddr = "";
		var AppSKey = "";
		var NwkSKey = "";
		
  	  	if (error !== null) {
			callback({
				AppEUI: AppEUI,
				DevEUI: DevEUI,
				DevAddr: DevAddr,
				AppSKey: AppSKey,
				NwkSKey: NwkSKey
			});

			return;
  	  	}
		
		var found = (/.*AppEUI\:\s(.*)/gi).exec(stdout);
		if (found) {
			AppEUI = found[1];
		}

		found = (/.*DevEUI\:\s(.*)/gi).exec(stdout);
		if (found) {
			DevEUI = found[1];
		}

		found = (/.*DevAddr\:\s(.*)/gi).exec(stdout);
		if (found) {
			DevAddr = found[1];
		}

		found = (/.*AppSKey\:\s(.*)/gi).exec(stdout);
		if (found) {
			AppSKey = found[1];
		}

		found = (/.*NwkSKey\:\s(.*)/gi).exec(stdout);
		if (found) {
			NwkSKey = found[1];
		}
				
		callback({
			AppEUI: AppEUI,
			DevEUI: DevEUI,
			DevAddr: DevAddr,
			AppSKey: AppSKey,
			NwkSKey: NwkSKey
		});
	});
};

function createTTNDevice(node, callback) {
	child = exec("ttnctl devices register " + node, function (error, stdout, stderr) {
  	  	if (error !== null) {
			getTTNDeviceInfo(node, function(info) {
				callback(info);
				return;
			});

			return;
  	  	}
		
		child = exec("ttnctl devices personalize " + node, function (error, stdout, stderr) {
	  	  	if (error !== null) {
				getTTNDeviceInfo(node, function(info) {
					callback(info);
					return;
				});
	  	  	}
		
			getTTNDeviceInfo(node, function(info) {
				callback(info);
			});
		});	
	});	
};

TTNCallback = function(message) {
	// Process your message here
	console.log(message);
};

// Topics
//
// /ide/user/login
// /ide/user/login_info
MQTTCallback = function(topic, message) {
	var topicParts = topic.split("/");
	
	if (topicParts[1] != "ide") return;
	
	var user = topicParts[2];
	var op = topicParts[3];

	if (op == "login") {
		createTTNDevice(user, function(info) {
			MQTTPublish("/ide/" + node + "/login_info", JSON.stringify({
				TTN: info
			}));
		});
	}
};