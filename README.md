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

The firmware exposes several getters over Mongoose OS RPC subsystem.

*    ***Channel.GetCurrent***: 
*    ***Channel.GetFrequency***: 
*    ***Channel.GetCumulative***: 
*    ***Channel.Clear***:
*    ***State.Read***:
*    ***State.Write***:
