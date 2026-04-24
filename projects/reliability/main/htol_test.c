// Copyright 2020-2025 Beken
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "htol_test.h"

extern bool htol_is_enabled(void);

void htol_reliability_test_start(void)
{
#if CONFIG_HTOL_RELIABILITY_TEST
	if (htol_is_enabled()) {
#if CONFIG_PWM_TEST
		extern void start_bk_pwm_breathing_led_self_test_thread(void);
		start_bk_pwm_breathing_led_self_test_thread();
#endif
#if CONFIG_PHY_TEST
		extern void start_bk_phy_test_htol_test_thread(void);
		start_bk_phy_test_htol_test_thread();
#endif
	}
#endif
}
