#include "rpc.h"
#include "modbus.h"

static void rpc_log(struct mg_rpc_request_info *ri, struct mg_str args) {
  LOG(LL_INFO,
      ("tag=%.*s src=%.*s method=%.*s args='%.*s'", ri->tag.len, ri->tag.p,
       ri->src.len, ri->src.p, ri->method.len, ri->method.p, args.len, args.p));
}

static bool valid_idx(struct mg_str args, struct mg_rpc_request_info *ri,
                      int *idx) {
  json_scanf(args.p, args.len, ri->args_fmt, idx);
  if (*idx < -1 || *idx >= PDU_NUM_CHANNELS) {
    mg_rpc_send_errorf(ri, 400, "idx must be between 0..%d or -1 for all",
                       PDU_NUM_CHANNELS - 1);
    return false;
  }
  return true;
}

static void rpc_channel_get_current(struct mg_rpc_request_info *ri,
                                    void *cb_arg, struct mg_rpc_frame_info *fi,
                                    struct mg_str args) {
  int idx = -1;
  double amps;

  rpc_log(ri, args);
  if (!valid_idx(args, ri, &idx)) return;

  if (idx != -1) {
    if (!modbus_channel_get_current(idx, &amps)) {
      mg_rpc_send_errorf(ri, 500, "could not get current for channel %d", idx);
      return;
    } else {
      mg_rpc_send_responsef(ri, "{retval: %B, idx: %d, current: %.2f}", true,
                            idx, amps);
      return;
    }
  } else {
    char rpl_str[1000];
    struct json_out rpl = JSON_OUT_BUF(rpl_str, sizeof(rpl_str));
    json_printf(&rpl, "{ retval: %B, current: [", true);
    for (idx = 0; idx < PDU_NUM_CHANNELS; idx++) {
      if (idx > 0) json_printf(&rpl, ",");
      amps = 0;
      if (!modbus_channel_get_current(idx, &amps)) {
        mg_rpc_send_errorf(ri, 500, "could not get current for channel %d",
                           idx);
        return;
      }
      json_printf(&rpl, "%.2f", amps);
    }
    json_printf(&rpl, "] }");
    mg_rpc_send_responsef(ri, "%s", rpl_str);
    return;
  }

  /* NOTREACH */
  ri = NULL;
  return;
}

static void rpc_channel_get_ratio(struct mg_rpc_request_info *ri, void *cb_arg,
                                  struct mg_rpc_frame_info *fi,
                                  struct mg_str args) {
  int idx = -1;
  uint16_t ratio;

  rpc_log(ri, args);
  if (!valid_idx(args, ri, &idx)) return;

  if (idx != -1) {
    if (!modbus_channel_get_ratio(idx, &ratio)) {
      mg_rpc_send_errorf(ri, 500, "could not get ratio for channel %d", idx);
      return;
    } else {
      mg_rpc_send_responsef(ri, "{retval: %B, idx: %d, ratio: %d}", true, idx,
                            ratio);
      return;
    }
  } else {
    char rpl_str[1000];
    struct json_out rpl = JSON_OUT_BUF(rpl_str, sizeof(rpl_str));
    json_printf(&rpl, "{ retval: %B, ratio: [", true);
    for (idx = 0; idx < PDU_NUM_CHANNELS; idx++) {
      if (idx > 0) json_printf(&rpl, ",");
      ratio = 0;
      if (!modbus_channel_get_ratio(idx, &ratio)) {
        mg_rpc_send_errorf(ri, 500, "could not get ratio for channel %d", idx);
        return;
      }
      json_printf(&rpl, "%d", ratio);
    }
    json_printf(&rpl, "] }");
    mg_rpc_send_responsef(ri, "%s", rpl_str);
    return;
  }

  /* NOTREACH */
  ri = NULL;
  return;
}

static void rpc_channel_get_frequency(struct mg_rpc_request_info *ri,
                                      void *cb_arg,
                                      struct mg_rpc_frame_info *fi,
                                      struct mg_str args) {
  int idx = -1;
  double hertz;

  rpc_log(ri, args);
  if (!valid_idx(args, ri, &idx)) return;

  if (idx != -1) {
    if (!modbus_channel_get_freq(idx, &hertz)) {
      mg_rpc_send_errorf(ri, 500, "could not get frequency for channel %d",
                         idx);
      return;
    } else {
      mg_rpc_send_responsef(ri, "{retval: %B, idx: %d, frequency: %.2f}", true,
                            idx, hertz);
      return;
    }
  } else {
    char rpl_str[1000];
    struct json_out rpl = JSON_OUT_BUF(rpl_str, sizeof(rpl_str));
    json_printf(&rpl, "{ retval: %B, frequency: [", true);
    for (idx = 0; idx < PDU_NUM_CHANNELS; idx++) {
      if (idx > 0) json_printf(&rpl, ",");
      hertz = 0;
      if (!modbus_channel_get_freq(idx, &hertz)) {
        mg_rpc_send_errorf(ri, 500, "could not get frequency for channel %d",
                           idx);
        return;
      }
      json_printf(&rpl, "%.2f", hertz);
    }
    json_printf(&rpl, "] }");
    mg_rpc_send_responsef(ri, "%s", rpl_str);
    return;
  }

  /* NOTREACH */
  ri = NULL;
  return;
}

static void rpc_channel_get_kwh(struct mg_rpc_request_info *ri, void *cb_arg,
                                struct mg_rpc_frame_info *fi,
                                struct mg_str args) {
  int idx = -1;
  double kwh;

  rpc_log(ri, args);
  if (!valid_idx(args, ri, &idx)) return;

  if (idx != -1) {
    if (!modbus_channel_get_kwh(idx, &kwh)) {
      mg_rpc_send_errorf(ri, 500, "could not get kWh for channel %d", idx);
      return;
    } else {
      mg_rpc_send_responsef(ri, "{retval: %B, idx: %d, kwh: %.2f}", true, idx,
                            kwh);
      return;
    }
  } else {
    char rpl_str[1000];
    struct json_out rpl = JSON_OUT_BUF(rpl_str, sizeof(rpl_str));
    json_printf(&rpl, "{ retval: %B, kwh: [", true);
    for (idx = 0; idx < PDU_NUM_CHANNELS; idx++) {
      if (idx > 0) json_printf(&rpl, ",");
      kwh = 0;
      if (!modbus_channel_get_kwh(idx, &kwh)) {
        mg_rpc_send_errorf(ri, 500, "could not get kWh for channel %d", idx);
        return;
      }
      json_printf(&rpl, "%.2f", kwh);
    }
    json_printf(&rpl, "] }");
    mg_rpc_send_responsef(ri, "%s", rpl_str);
    return;
  }

  /* NOTREACH */
  ri = NULL;
  return;
}

static void rpc_channel_clear(struct mg_rpc_request_info *ri, void *cb_arg,
                              struct mg_rpc_frame_info *fi,
                              struct mg_str args) {
  int idx = -1;

  rpc_log(ri, args);
  if (!valid_idx(args, ri, &idx)) return;

  if (idx != -1) {
    if (!modbus_channel_clear(idx, false)) {
      mg_rpc_send_errorf(ri, 500, "could not clear channel %d", idx);
      return;
    } else {
      mg_rpc_send_responsef(ri, "{retval: %B}", true);
      return;
    }
  } else {
    for (idx = 0; idx < PDU_NUM_CHANNELS; idx++) {
      if (!modbus_channel_clear(idx, false)) {
        mg_rpc_send_errorf(ri, 500, "could not clear channel %d", idx);
        return;
      }
    }
    mg_rpc_send_responsef(ri, "{retval: %B}", true);
    return;
  }

  /* NOTREACH */
  ri = NULL;
  return;
}

static void rpc_state_read(struct mg_rpc_request_info *ri, void *cb_arg,
                           struct mg_rpc_frame_info *fi, struct mg_str args) {
  rpc_log(ri, args);
  if (!modbus_state_read(STATE_FILENAME)) {
    mg_rpc_send_errorf(ri, 500, "FAILED");
  } else {
    mg_rpc_send_responsef(ri, "{retval: %B}", true);
  }
  ri = NULL;
  return;
}

static void rpc_state_write(struct mg_rpc_request_info *ri, void *cb_arg,
                            struct mg_rpc_frame_info *fi, struct mg_str args) {
  rpc_log(ri, args);
  if (!modbus_state_write()) {
    mg_rpc_send_errorf(ri, 500, "FAILED");
  } else {
    mg_rpc_send_responsef(ri, "{retval: %B}", true);
  }
  ri = NULL;
  return;
}

void rpc_init() {
  struct mg_rpc *c = mgos_rpc_get_global();

  mg_rpc_add_handler(c, "Channel.GetCurrent", "{idx: %d}",
                     rpc_channel_get_current, NULL);
  mg_rpc_add_handler(c, "Channel.GetFrequency", "{idx: %d}",
                     rpc_channel_get_frequency, NULL);
  mg_rpc_add_handler(c, "Channel.GetRatio", "{idx: %d}", rpc_channel_get_ratio,
                     NULL);
  mg_rpc_add_handler(c, "Channel.GetkWh", "{idx: %d}", rpc_channel_get_kwh,
                     NULL);
  mg_rpc_add_handler(c, "Channel.Clear", "{idx: %d}", rpc_channel_clear, NULL);
  mg_rpc_add_handler(c, "State.Read", "{}", rpc_state_read, NULL);
  mg_rpc_add_handler(c, "State.Write", "{}", rpc_state_write, NULL);
}
