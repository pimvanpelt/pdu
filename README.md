# Modbus PDU

This is a simple firmware that connects over serial to a 16 channel Modbus current
sensor like [this one](https://www.aliexpress.com/item/4000131557448.html) (search
for "Multi-channel 16-channel AC current frequency measurement acquisition module
RS485 sensor transmitter MODBUS-RTU" on Aliexpress for good examples). 

The ESP32 connects using serial to the serial output of the current sensor, before
it is turned into RS-485. It is then no longer advised to use RS-485. It will read
the current sensor outputs once every 5 seconds, and integrate the total power use
per channel. It will periodically save its state to flash, in order to survive
power failures, reboots and firmware updates.

## Prometheus Metrics

TODO(pim) --

The firmware exposes all channel information using the Prometheus exposition
format, ready for scraping and integration into a network monitoring system.
It also exports several modbus statistics (notably number of reads, number of
responses, and number of invalid responses).

## API

The firmware exposes several getters over Mongoose OS RPC subsystem. See
`RPC.List` to enumerate them all, and `RPC.Describe '{"name": ... }'` to show
the call proto.

The `State` RPC Service either writes the statefile describing the current
PDU state to flash, or reads it back from flash into memory.

The `Channel` RPC Service takes either no arguments, in which case report for
all channels is given as a list, or if a parameter `{idx: 0 }` is present, only
that one channel is returned as a scalar. Note that channels are run from 0..15.
When calling `Channel.Clear`, care should be taken to also persist the state to
flash with `State.Write`, so that restarts do not inadvertently restore old
state.

Examples:

```
$ mos call Channel.GetCurrent '{"idx": 13 }'
{ "retval": true, "idx": 13, "current": 0.22 }

$ mos call Channel.GetkWh
{ "retval": true, "kwh": [ 14.51, 0.00, 0.00, 0.00, 0.00, 7.23,
  0.04, 1910.34, 0.00, 0.00, 0.00, 0.35, 4.62, 4.77, 0.00, 0.00 ] }

$ mos call mos call State.Write
{ "retval": true }

$ mos call Channel.Clear '{"idx": 15 }' && mos call State.Write
{ "retval": true }
{ "retval": true }
```

## PubSub

The firmware sends periodical updates to a configured `MQTT` server. The
following events are sent:

*    Startup: A message is sent to `/mongoose/broadcast/stat/id` with
     information about the microcontroller and firmware.
*    Periodical: A message is sent to `${device_id}/stat/pdu` with
     information about the channels in JSON format.

Examples:
```
TODO(pim)
```
