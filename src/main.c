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
#include "mgos.h"
#include "modbus.h"

static void button_handler(int pin, void *args) {
  LOG(LL_INFO, ("Button pressed, persisting state"));
  modbus_state_write();
}

enum mgos_app_init_result mgos_app_init(void) {
  modbus_init(mgos_sys_config_get_pdu_modbus_interval(),
              mgos_sys_config_get_pdu_state_interval(), STATE_FILENAME);
  mgos_gpio_set_button_handler(39, MGOS_GPIO_PULL_UP, MGOS_GPIO_INT_EDGE_NEG,
                               100, button_handler, NULL);

  return MGOS_APP_INIT_SUCCESS;
}
