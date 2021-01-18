/*
 * Copyright 2021 Pim van Pelt <pim@ipng.nl>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#pragma once

#include <stdbool.h>
#include <stdint.h>
#include "mgos.h"
#include "mgos_modbus.h"

#define PDU_NUM_CHANNELS 16
#define PDU_VOLTAGE 232.

#define MODBUS_READ_STALE_THRESH \
  30  // Do not integrate measurements more than X seconds apart.

#define MODBUS_TIMER_DEFAULT 5  // Seconds
#define STATE_TIMER_DEFAULT 24  // Hours
#define STATE_FILENAME "state-v0.bin"

struct pdu_channel {
  double last_cleared_time;
  double ampere_seconds_total;
  uint16_t raw_current;    // units of 0.01A
  uint16_t raw_frequency;  // units of 0.1Hz
  uint16_t raw_ratio;      // CT ratio 1:n
  uint8_t __pad[2];
};

struct pdu {
  double last_read_time;
  double last_save_time;
  uint16_t state_write_hours;  // Interval for the state write timer (in hours)
  uint16_t modbus_read_secs;  // Interval for the modbus read timer (in seconds)
  uint16_t pdu_version;       // number 650 as 6.5.0
  uint16_t pdu_current_range[2];  // Channel range in Ampere (40), first byte
                                  // A-H, second byte I-P.
  uint8_t pdu_build_year;         // Factory date (year)
  uint8_t pdu_build_month;        // Factory date (month)
  char state_filename[40];
  struct pdu_channel pdu_channel[PDU_NUM_CHANNELS];
};

/* modbus_init(): Initialize the modbus subsystem, start a timer each
 * modbus_read_secs that reads from the sensor, and a timer each
 * state_write_hours that persists the running state on disk under the given
 * file name in state_filename.
 *
 * Returns: true if successful, false otherwise.
 */
bool modbus_init(uint16_t modbus_read_secs, uint16_t state_write_hours,
                 const char *state_filename);

/* modbus_state_read(): (re)read the cache file with PDU channel counters from
 * disk. This allows the system to continue where it left off upon reboot /
 * crash / power failure.
 * Note that the state file contains the state_filename as well, and it must be
 * the same given in the state_filename argument for the read to be successful.
 *
 * Returns: true if successful, false otherwise.
 */
bool modbus_state_read(const char *state_filename);

/* modbus_state_write(): Persist the pdu struct including pdu_channel
 * information to disk. This allows the system to make a periodic backup of its
 * state, so that it can continue where it left off upon reboot / crash / power
 * failure.
 *
 * The filename written to is taken from the modbus_init() argument.
 *
 * Returns: true if successful, false otherwise.
 */
bool modbus_state_write(void);

/* modbus_channel_get_freq(): Return the frequency of the specificied channel
 * (0..15) in units of Hertz (eg 50.0)
 *
 * Returns: true if successful, false otherwise.
 */
bool modbus_channel_get_freq(uint8_t chan, double *hertz);

/* modbus_channel_get_current(): Return the current of the specificied channel
 * (0..15) in units of Amperes (eg 0.55). The resolution is 0.01 Ampere (10 mA).
 *
 * Returns: true if successful, false otherwise.
 */
bool modbus_channel_get_current(uint8_t chan, double *amperes);

/* modbus_channel_get_ratio(): Return the CT turn-ratio of the specificied
 * channel (0..15), eg. 1000 for 1:1000.
 *
 * Returns: true if successful, false otherwise.
 */
bool modbus_channel_get_ratio(uint8_t chan, uint16_t *ratio);

/* modbus_channel_get_kwh(): Return the consumed power of the specificied
 * channel (0..15) since the last channel reset, in kilowatthours (kWh).
 *
 * Returns: true if successful, false otherwise.
 */
bool modbus_channel_get_kwh(uint8_t chan, double *kwh);

/* modbus_channel_get_freq(): Return the timestamp of the specificied channel's
 * (0..15) last clear time in type mgos_uptime, or 0 if it has never been
 * cleared.
 *
 * Returns: true if successful, false otherwise.
 */
bool modbus_channel_get_last_clear(uint8_t chan, double *last_clear);

/* modbus_channel_get_freq(): Return the timestamp of the specificied channel's
 * (0..15) last read time in type mgos_uptime, or 0 if it has never been read.
 *
 * Returns: true if successful, false otherwise.
 */
bool modbus_channel_get_last_read(uint8_t chan, double *last_read);

/* modbus_channel_clear(): Clear the cumulative counter on the specified
 * channel (0..15), and if the persist flag is true, also write the state
 * to disk.
 *
 * Returns: true if successful, false otherwise.
 */
bool modbus_channel_clear(uint8_t chan, bool persist);
