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

#include "mgos.h"
#include "mgos_mqtt.h"

#define MQTT_TOPIC_PREFIX ""
#define MQTT_TOPIC_BROADCAST_CMD "/mongoose/broadcast"
#define MQTT_TOPIC_BROADCAST_STAT "/mongoose/broadcast/stat"

void mqtt_init();
void mqtt_publish_stat(const char *stat, const char *fmt, ...);
