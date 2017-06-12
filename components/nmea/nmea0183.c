/*
 * Lua RTOS, NMEA parser
 *
 * Copyright (C) 2015 - 2017
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

#include "sdkconfig.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include <sys/syslog.h>

static char *GPGGA = "GPGGA";  // GPGGA sentence string
//static char *GPRMC = "GPRMC";  // GPRMC sentence string
//static int  date_updated = 0;  // 0 = date is not updated yet, 1 = updated

// Last position data
static double lat, lon;
static int sats;
static int new_pos = 0;

// As parser computes checksum incrementally, this function computes
// the checksum that corresponds to the nma sentence string
static int nmea_initial_checksum(char *c) {
    int computed_checksum = 0;

    while (*c) {
        computed_checksum ^= *c;
        c++;
    }

    computed_checksum ^= ',';

    return computed_checksum;
}

// Converts a geolocation expressed in � ' '' to decimal
double nmea_geoloc_to_decimal(char *token) {
    double val 	   = 0.0;
    double degrees = 0.0;
    double minutes = 0.0;
    double seconds = 0.0;

    char *current = token;

    while (*current) {
        if (*current == '.') {
            // xxxx.xxxx
            //     |
            *current = 0; // Add end string for get minutes

            // Go to beginning of minutes
            // Get minutes value
            current--;
            current--;

            //minutes = atof(current);
            minutes = (double)atoi(current);

            // xxxx.xxxx
            //   |
            *current = 0; // Add end string for get degrees

            //degrees = atof(token);
            degrees = (double)atoi(token);

            // Get second value
            current++;
            *current = '0';  // Put 0 for convert seconds from ascci to double
            current++;	     // Put . for convert seconds from ascci to double
            *current = '.';
            current--;

            //seconds = atof(current);

            current++;
            current++;
            seconds = (double)atoi(current) / (double)1000000;

            // Do the conversion
            val = degrees + ((minutes + seconds) / 60);

            // Exit while
            break;
        }
        current++;
    }

    return val;
}

// Global Positioning System Fix Data
// $GPGGA,hhmmss.ss,llll.ll,a,yyyyy.yy,a,x,xx,x.x,x.x,M,x.x,M,x.x,xxxx*hh
//
// 1    = UTC of Position
// 2    = Latitude
// 3    = N or S
// 4    = Longitude
// 5    = E or W
// 6    = GPS quality indicator (0=invalid; 1=GPS fix; 2=Diff. GPS fix)
// 7    = Number of satellites in use [not those in view]
// 8    = Horizontal dilution of position
// 9    = Antenna altitude above/below mean sea level (geoid)
// 10   = Meters  (Antenna height unit)
// 11   = Geoidal separation (Diff. between WGS-84 earth ellipsoid and
//        mean sea level.  -=geoid is below WGS-84 ellipsoid)
// 12   = Meters  (Units of geoidal separation)
// 13   = Age in seconds since last update from diff. reference station
// 14   = Diff. reference station ID#
// 15   = Checksum
static void nmea_GPGGA(char *sentence) {
    int    valid = 1; // It's a valid position?

    int seq = 0;      // Current nmea sentence field (0 = first after nmea command)
    char *c;
    char *token;

    int checksum = 0;
    int computed_checksum = nmea_initial_checksum(GPGGA);

    new_pos = 0;

    token = c = sentence;
    while (*c) {
        if (*c == ',') {
            computed_checksum ^= *c;

            *c++ = 0;

            if (seq == 1) { // Latitude
                lat = nmea_geoloc_to_decimal(token);
            }

            if (seq == 2) { // N or S
                if (strcmp("S",token) == 0) {
                    lat = lat * -1;
                }
            }

            if (seq == 3) { // Longitude
                lon = nmea_geoloc_to_decimal(token);
            }

            if (seq == 4) { // E or W
                if (strcmp("W",token) == 0) {
                    lon = lon * -1;
                }
            }

            if (seq == 5) {	// GPS quality indicator (0=invalid; 1=GPS fix; 2=Diff. GPS fix)
                int quality = atoi(token);

                valid = ((quality == 1) || (quality == 2));
            }

            if (seq == 6) {	// Number of satellites in use [not those in view]
                sats = atoi(token);
            }

            token = c;
            seq++;
        } else {
            if (*c == '*') {
                c++;
                token = c;
                break;
            } else {
                computed_checksum ^= *c;
                c++;
            }
        }
    }

    checksum = (int)strtol(token, NULL, 16);

    if (checksum == computed_checksum) {
        if (valid) {
            new_pos = 1;
        }
    }
}

#if 0
// Recommended minimum specific GPS/Transit data
// $GPRMC,081836,A,3751.65,S,14507.36,E,000.0,360.0,130998,011.3,E*62
//
// 1    = UTC of position fix
// 2    = Data status (V=navigation receiver warning)
// 3    = Latitude of fix
// 4    = N or S
// 5    = Longitude of fix
// 6    = E or W
// 7    = Speed over ground in knots
// 8    = Track made good in degrees True
// 9    = UT date
// 10   = Magnetic variation degrees (Easterly var. subtracts from true course)
// 11   = E or W
// 12   = Checksum
static void nmea_GPRMC(char *sentence) {
    int seq = 0;
    char *c;
    char *token;

    int checksum = 0;
    int computed_checksum = nmea_initial_checksum(GPRMC);

    char  tmp[3];
    int  month,day,hours,minutes,seconds;
    uint16_t year;

    token = c = sentence;
    while (*c) {
        if (*c == ',') {
            computed_checksum ^= *c;

            *c++ = 0;

            if (seq == 0) {	// UTC of position fix
                tmp[0] = *(token + 0);
                tmp[1] = *(token + 1);
                tmp[2] = 0;
                hours = atoi(tmp);

                tmp[0] = *(token + 2);
                tmp[1] = *(token + 3);
                tmp[2] = 0;
                minutes = atoi(tmp);

                tmp[0] = *(token + 4);
                tmp[1] = *(token + 5);
                tmp[2] = 0;
                seconds = atoi(tmp);
            }

            if (seq == 8) {	// UT date
                tmp[0] = *(token + 0);
                tmp[1] = *(token + 1);
                tmp[2] = 0;
                day = atoi(tmp);

                tmp[0] = *(token + 2);
                tmp[1] = *(token + 3);
                tmp[2] = 0;
                month = atoi(tmp);

                tmp[0] = *(token + 4);
                tmp[1] = *(token + 5);
                tmp[2] = 0;
                year = atoi(tmp);
            }

            token = c;
            seq++;
        } else {
            if (*c == '*') {
                c++;
                token = c;
                break;
            } else {
                computed_checksum ^= *c;
                c++;
            }
        }
    }

    checksum = (int)strtol(token, NULL, 16);
    if (checksum == computed_checksum) {
        if (year >= 15) {
            date_updated = 1;

            time_t newtime;
            struct tm tms;

            memset(&tms, 0, sizeof(struct tm));

            tms.tm_hour = hours;
            tms.tm_min = minutes;
            tms.tm_sec = seconds;
            tms.tm_year = year;
            tms.tm_mon = month;
            tms.tm_mday = day;
            newtime = mktime(&tms);

            settimeofday((const time_t *)&newtime);
            syslog(LOG_DEBUG, "nmea01843 setting system time to %d-%d-%d %d:%d:%d",year,month,day,hours,minutes,seconds);
        }
    }
}
#endif

void nmea_parse(char *sentence) {
    char *c;
    char *token;

    token = c = sentence;
    while (*c) {
        if ((*c == ',') || (*c == '*')) {
            *c++ = 0;

            // Skip $
            if (*token == '$') {
                token++;
            }

            if (strncmp(GPGGA, token, 5) == 0) {
                nmea_GPGGA(c);
            //} else if ((strncmp(GPRMC, token, 5) == 0) && (!date_updated)) {
            //    nmea_GPRMC(c);
            } else {
                break;
            }

            token = c;
        } else {
            c++;
        }
    }
}

int nmea_new_pos() {
    return new_pos;
}

double nmea_lon() {
    return lon;
}

double nmea_lat() {
    return lat;
}

int nmea_sats() {
    return sats;
}
