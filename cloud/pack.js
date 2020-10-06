/*
 * Whitecat Ecosystem, pack / unpack functions for node
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

// Test if value is an integer
function isInt(value) {
    var er = /^-?[0-9]+$/;

    return er.test(value);
}

function byteToByteArray(number) {
    var buffer = new ArrayBuffer(1);         
    var longNum = new Int8Array(buffer);

    longNum[0] = number;

    return Array.from(new Int8Array(buffer));
}

// Convert the JS integer (64 bits) representation to
// a 32 bit integer number representation
function longToByteArray(number) {
    var buffer = new ArrayBuffer(4);         
    var longNum = new Int32Array(buffer);

    longNum[0] = number;

    return Array.from(new Int8Array(buffer));
}

// Convert the JS number (64 bits) representation to
// a 32 bit float number representation
function doubleToByteArray(number) {
    var buffer = new ArrayBuffer(4);         
    var longNum = new Float32Array(buffer);

    longNum[0] = number;

    return Array.from(new Int8Array(buffer));
}

function Bytes2Float32(bytes) {
	bytes = parseInt(bytes);
	
    var sign = (bytes & 0x80000000) ? -1 : 1;
    var exponent = ((bytes >> 23) & 0xFF) - 127;
    var significand = (bytes & ~(-1 << 23));

    if (exponent == 128) 
        return sign * ((significand) ? Number.NaN : Number.POSITIVE_INFINITY);

    if (exponent == -127) {
        if (significand == 0) return sign * 0.0;
        exponent = -126;
        significand /= (1 << 22);
    } else significand = (significand | (1 << 23)) / (1 << 23);

    return sign * significand * Math.pow(2, exponent);   
}

unpack = function(payload) {
	var byte = "";
	var fields = 0;
	var pos = 0;
	var hn = 0;
	var ln = 0;
	var types = [];
	
	// Get number of fields
	fields = payload[pos++];
	
	// Parse fields
	for(var field=0;field < fields;field++) {
		byte  = payload[pos++];

		hn = ((byte & 0xf0) >> 4);
		types.push(hn);		

		field++;
		ln = (byte & 0x0f);
		if (field < fields) {
			types.push(ln);		
		}		
	}
		
	// Get values
	var value;
	var values = [];
	
	for(var i = 0;i < types.length;i++) {
		value = "";
		switch (types[i]) {
			case 0:
				// Float, 4 bytes
				value  = payload[pos++];
				value |= payload[pos++] << 8;
				value |= payload[pos++] << 16;
				value |= payload[pos++] << 24;
				
				values.push(Bytes2Float32(value));

				break;
			case 1:
				// Int, 4 bytes
				value  = payload[pos++];
				value |= payload[pos++] << 8;
				value |= payload[pos++] << 16;
				value |= payload[pos++] << 24;
				
				values.push(value);
				break;
			case 2:
				// Nil
				values.push(null);
				break;
			case 3:
				// Bool
				values.push(payload[pos++] == 1);
				break;
			case 4:
				// string
				var str = '';
				var current;
				
				value  = payload[pos++];

				while (value != 0) {
					str += String.fromCharCode(value); 
					value  = payload[pos++];
				}

				values.push(str);
				
				break;
		}	
	}
	
	return values;
}

pack = function() {
	var fields = [];
	var types = [];
	
	for (var i = 0; i < arguments.length; i++) {	
		fields.push(arguments[i]);
		types.push(typeof arguments[i]);
	}
	
	// Pack number of fields
	var buffer = new Buffer(byteToByteArray(fields.length));
	
	// Pack field types
	var type = "";
	
	for (var i = 0; i < types.length; i++) {
		if (types[i] == 'number') {	
			if (isInt(fields[i])) {
				type += "1";		
			} else {
				type += "0";						
			}		
		} else if (types[i] == 'boolean') {
			type += "3";				
		} else if (types[i] == 'string') {
			type += "4";				
		} else if (types[i] == 'object') {
			type += "2";				
		}
		
		if (type.length == 2) {
			buffer = Buffer.concat([buffer, new Buffer(byteToByteArray(parseInt("0x" + type, 16)))]);			
			type = "";
		}
	}

	if (type.length == 1) {
		type = type + "0";
		buffer = Buffer.concat([buffer, new Buffer(byteToByteArray(parseInt("0x" + type, 16)))]);			
	}
	
	var bytes;

	// Pack field value
	for (var i = 0; i < types.length; i++) {
		if (types[i] == 'number') {	
			if (isInt(fields[i])) {
				bytes = longToByteArray(fields[i]);				
			} else {
				bytes = doubleToByteArray(fields[i]);
			}		
			
			buffer = Buffer.concat([buffer, new Buffer(bytes)]);							
		} else if (types[i] == 'boolean') {
			if (fields[i]) {
				buffer = Buffer.concat([buffer, new Buffer(byteToByteArray(1))]);							
			} else {
				buffer = Buffer.concat([buffer, new Buffer(byteToByteArray(0))]);							
			}
		} else if (types[i] == 'string') {
			for (var j = 0;j < fields[i].length;j++) {
				buffer = Buffer.concat([buffer, new Buffer(byteToByteArray(fields[i][j].charCodeAt(0)))]);							
			}
			buffer = Buffer.concat([buffer, new Buffer(byteToByteArray(0))]);							
		} else if (types[i] == 'object') {
			if (fields[i] == null) {
				buffer = Buffer.concat([buffer, new Buffer(0)]);							
			}
		}		
	}	
	
	return buffer;
}