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
#include "modbus.h"

static struct pdu s_pdu;

static uint64_t s_modbus_reads = 0;
static uint64_t s_modbus_responses = 0;
static uint64_t s_modbus_responses_invalid = 0;

static double ampsecs2kwh(double ampere_seconds) {
  // amp*sec * volts = Watts*sec
  // Watts*sec / 3600 = Watts/hr
  // Watts/hr / 1000 = kWh
  return (ampere_seconds * PDU_VOLTAGE) / 3600000.;
}

static void mb_read_response_handler(uint8_t status,
                                     struct mb_request_info mb_ri,
                                     struct mbuf response, void *param) {
  uint8_t modbus_address, modbus_function, modbus_datalen;
  double last_read_time;
  double amps_total = 0;
  double amp_secs_total = 0;
  int channels_active = 0;

  s_modbus_responses++;
  if (status != RESP_SUCCESS) {
    s_modbus_responses_invalid++;
    LOG(LL_ERROR, ("Invalid response: status=%d", status));
    return;
  }
  modbus_address = response.buf[0];
  modbus_function = response.buf[1];
  modbus_datalen = response.buf[2];
  if (modbus_address != 1 || modbus_function != 3 || modbus_datalen != 0x70) {
    s_modbus_responses_invalid++;
    LOG(LL_ERROR, ("Invalid response: address=%d function=%d datalen=%d",
                   modbus_address, modbus_function, modbus_datalen));
    return;
  }
  last_read_time = s_pdu.last_read_time;
  s_pdu.last_read_time = mg_time();
  s_pdu.pdu_version = (response.buf[3] << 8) + response.buf[4];
  s_pdu.pdu_current_range[0] = (response.buf[5] << 8) + response.buf[6];
  s_pdu.pdu_current_range[1] = (response.buf[7] << 8) + response.buf[8];
  s_pdu.pdu_build_month = response.buf[11];
  s_pdu.pdu_build_year = response.buf[12];
  LOG(LL_DEBUG, ("Response: PDU version %d (build %d.%d, range=(%dA,%dA))",
                 s_pdu.pdu_version, s_pdu.pdu_build_year, s_pdu.pdu_build_month,
                 s_pdu.pdu_current_range[0], s_pdu.pdu_current_range[1]));

  for (int i = 0; i < PDU_NUM_CHANNELS; i++) {
    s_pdu.pdu_channel[i].raw_current =
        (response.buf[(8 + i) * 2 + 3] << 8) + response.buf[(8 + i) * 2 + 4];
    s_pdu.pdu_channel[i].raw_frequency =
        (response.buf[(24 + i) * 2 + 3] << 8) + response.buf[(24 + i) * 2 + 4];
    s_pdu.pdu_channel[i].raw_ratio =
        (response.buf[(40 + i) * 2 + 3] << 8) + response.buf[(40 + i) * 2 + 4];
    LOG(LL_DEBUG,
        ("Channel %d: current=%d freq=%d ratio=%d", i,
         s_pdu.pdu_channel[i].raw_current, s_pdu.pdu_channel[i].raw_frequency,
         s_pdu.pdu_channel[i].raw_ratio));
  }
  double delta = s_pdu.last_read_time - last_read_time;
  if (delta > MODBUS_READ_STALE_THRESH) {
    LOG(LL_WARN,
        ("Modbus last read was %.f seconds ago, considering stale", delta));
    return;
  }
  amp_secs_total = 0;
  channels_active = 0;
  for (int i = 0; i < PDU_NUM_CHANNELS; i++) {
    double amp_secs;
    amp_secs = s_pdu.pdu_channel[i].raw_current * 0.01 * delta;
    amps_total += s_pdu.pdu_channel[i].raw_current * 0.01;
    s_pdu.pdu_channel[i].ampere_seconds_total += amp_secs;
    amp_secs_total += s_pdu.pdu_channel[i].ampere_seconds_total;
    if (s_pdu.pdu_channel[i].raw_frequency > 0 ||
        s_pdu.pdu_channel[i].raw_current > 0)
      channels_active++;
  }
  LOG(LL_INFO, ("PDU: channels=%d on=%d I=%.2fA P=%.2fW Pcum=%.2fkWh",
                PDU_NUM_CHANNELS, channels_active, amps_total,
                amps_total * PDU_VOLTAGE, ampsecs2kwh(amp_secs_total)));
}

static void modbus_timer(void *args) {
  LOG(LL_DEBUG, ("Reading modbus holding registers"));
  s_modbus_reads++;
  mb_read_holding_registers(1, 0, 56, mb_read_response_handler, NULL);
}

static void state_timer(void *args) {
  if (0 == strlen(s_pdu.state_filename)) return;

  LOG(LL_INFO, ("Persisting state to %s", s_pdu.state_filename));
  modbus_state_write();
}

bool modbus_init(uint16_t modbus_read_secs, uint16_t state_write_hours,
                 const char *state_filename) {
  if (!mgos_modbus_connect()) {
    LOG(LL_INFO, ("Unable to connect MODBUS"));
    return false;
  }
  memset(&s_pdu, 0, sizeof(struct pdu));

  strncpy(s_pdu.state_filename, state_filename, sizeof(s_pdu.state_filename));
  modbus_state_read(state_filename);

  s_pdu.modbus_read_secs = modbus_read_secs;
  mgos_set_timer(1000 * s_pdu.modbus_read_secs, MGOS_TIMER_REPEAT, modbus_timer,
                 NULL);

  s_pdu.state_write_hours = state_write_hours;
  mgos_set_timer(1000 * 3600 * s_pdu.state_write_hours, MGOS_TIMER_REPEAT,
                 state_timer, NULL);
  return true;
}

bool modbus_state_read(const char *state_filename) {
  int fd;
  struct pdu new_pdu;
  struct stat stat;
  int bytes_read = -1;
  bool ret = false;

  if (!state_filename) return false;

  LOG(LL_INFO, ("Reading state from %s", state_filename));
  fd = open(state_filename, O_RDONLY);
  if (fd < 0) {
    LOG(LL_ERROR,
        ("%s: Could not open(): %s", state_filename, strerror(errno)));
    goto exit;
  }
  if (0 != fstat(fd, &stat)) {
    LOG(LL_ERROR,
        ("%s: Could not stat(): %s", state_filename, strerror(errno)));
    goto exit;
  }
  if (stat.st_size != sizeof(struct pdu)) {
    LOG(LL_ERROR, ("%s: Size (%d) is not the PDU struct size (%d)",
                   state_filename, (int) stat.st_size, sizeof(struct pdu)));
    goto exit;
  }
  bytes_read = read(fd, &new_pdu, sizeof(struct pdu));
  if (bytes_read != sizeof(struct pdu)) {
    LOG(LL_ERROR, ("%s: Short read(), wanted %d got %d", state_filename,
                   sizeof(struct pdu), bytes_read));
    goto exit;
  }
  if (0 != memcmp(&new_pdu.state_filename, &s_pdu.state_filename,
                  strlen(s_pdu.state_filename))) {
    LOG(LL_ERROR, ("%s: Read state filename '%s' does not correspond",
                   state_filename, new_pdu.state_filename));
    goto exit;
  }

  // All sanity checks passed, let's consume the state file!
  memcpy(&s_pdu, &new_pdu, sizeof(struct pdu));
  LOG(LL_INFO, ("Read state from %s", state_filename));

  ret = true;
exit:
  if (fd >= 0) close(fd);
  return ret;
}

bool modbus_state_write(void) {
  bool ret = false;
  int bytes_written = -1;
  double last_save_time;
  int fd;

  if (0 == strlen(s_pdu.state_filename)) return false;
  last_save_time = s_pdu.last_save_time;

  fd = open(s_pdu.state_filename, O_WRONLY | O_CREAT | O_TRUNC);
  if (fd < 0) {
    LOG(LL_ERROR,
        ("%s: Could not open(): %s", s_pdu.state_filename, strerror(errno)));
    goto exit;
  }
  s_pdu.last_save_time = mg_time();
  bytes_written = write(fd, &s_pdu, sizeof(struct pdu));
  if (bytes_written != sizeof(struct pdu)) {
    LOG(LL_ERROR, ("%s: Short write(), wanted %d got %d", s_pdu.state_filename,
                   sizeof(struct pdu), bytes_written));
    goto exit;
  }
  LOG(LL_INFO, ("State persisted to %s", s_pdu.state_filename));

  ret = true;
exit:
  if (!ret) s_pdu.last_save_time = last_save_time;
  if (fd >= 0) close(fd);
  return ret;
}

bool modbus_channel_get_freq(uint8_t chan, double *hertz) {
  if (chan >= PDU_NUM_CHANNELS || !hertz) return false;
  *hertz = s_pdu.pdu_channel[chan].raw_frequency * 0.1;
  return true;
}

bool modbus_channel_get_current(uint8_t chan, double *amperes) {
  if (chan >= PDU_NUM_CHANNELS || !amperes) return false;
  *amperes = s_pdu.pdu_channel[chan].raw_current * 0.01;
  return true;
}

bool modbus_channel_get_ratio(uint8_t chan, uint16_t *ratio) {
  if (chan >= PDU_NUM_CHANNELS || !ratio) return false;
  *ratio = s_pdu.pdu_channel[chan].raw_ratio;
  return true;
}

bool modbus_channel_get_kwh(uint8_t chan, double *kwh) {
  if (chan >= PDU_NUM_CHANNELS || !kwh) return false;
  *kwh = ampsecs2kwh(s_pdu.pdu_channel[chan].ampere_seconds_total);
  return true;
}

bool modbus_channel_get_last_clear(uint8_t chan, double *last_clear) {
  if (chan >= PDU_NUM_CHANNELS || !last_clear) return false;
  *last_clear = s_pdu.pdu_channel[chan].last_cleared_time;
  return true;
}

bool modbus_channel_get_last_read(uint8_t chan, double *last_read) {
  if (chan >= PDU_NUM_CHANNELS || !last_read) return false;
  *last_read = s_pdu.last_read_time;
  return true;
}

bool modbus_channel_clear(uint8_t chan, bool persist) {
  if (chan >= PDU_NUM_CHANNELS) return false;

  LOG(LL_INFO, ("Clearing counters for channel %d", chan));
  memset(&s_pdu.pdu_channel[chan], 0, sizeof(struct pdu_channel));
  s_pdu.pdu_channel[chan].last_cleared_time = mg_time();

  if (persist) return modbus_state_write();

  return true;
}
