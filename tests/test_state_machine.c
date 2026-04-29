#include <stdio.h>
#include <assert.h>
#include "../firmware/bike-unit/components/alert_manager/include/alert_manager.h"
#include "../firmware/bike-unit/components/alert_manager/state_machine.c"

static int arm_count = 0, disarm_count = 0, warning_count = 0;
static int alert_count = 0, tracking_count = 0, siren_on_count = 0, siren_off_count = 0;

static void cb_arm(void) { arm_count++; }
static void cb_disarm(void) { disarm_count++; }
static void cb_warning(trigger_source_t s) { (void)s; warning_count++; }
static void cb_alert(trigger_source_t s) { (void)s; alert_count++; }
static void cb_tracking(void) { tracking_count++; }
static void cb_siren_on(void) { siren_on_count++; }
static void cb_siren_off(void) { siren_off_count++; }

static void reset_counts(void) {
    arm_count = disarm_count = warning_count = 0;
    alert_count = tracking_count = siren_on_count = siren_off_count = 0;
}

void test_basic_arm_disarm(void)
{
    alarm_sm_t sm;
    state_callbacks_t cbs = { .on_arm = cb_arm, .on_disarm = cb_disarm };
    sm_init(&sm, &cbs, 5000);
    reset_counts();

    assert(sm_get_state(&sm) == STATE_DISARMED);

    sm_process_event(&sm, EVT_ARM_CMD, 1000);
    assert(sm_get_state(&sm) == STATE_ARMED);
    assert(arm_count == 1);

    sm_process_event(&sm, EVT_DISARM_CMD, 2000);
    assert(sm_get_state(&sm) == STATE_DISARMED);
    assert(disarm_count == 1);

    printf("  ✓ basic arm/disarm\n");
}

void test_full_alert_sequence(void)
{
    alarm_sm_t sm;
    state_callbacks_t cbs = {
        .on_arm = cb_arm, .on_disarm = cb_disarm,
        .on_warning = cb_warning, .on_alert = cb_alert,
        .on_tracking = cb_tracking,
        .on_siren_on = cb_siren_on, .on_siren_off = cb_siren_off,
    };
    sm_init(&sm, &cbs, 5000);
    reset_counts();

    sm_process_event(&sm, EVT_ARM_CMD, 0);
    assert(sm_get_state(&sm) == STATE_ARMED);

    sm_set_trigger(&sm, TRIGGER_MOTION);
    sm_process_event(&sm, EVT_SENSOR_TRIGGER, 1000);
    assert(sm_get_state(&sm) == STATE_WARNING);
    assert(warning_count == 1);

    sm_process_event(&sm, EVT_THREAT_CONFIRMED, 3000);
    assert(sm_get_state(&sm) == STATE_ALERT);
    assert(alert_count == 1);
    assert(siren_on_count == 1);

    sm_process_event(&sm, EVT_GPS_MOVING, 10000);
    assert(sm_get_state(&sm) == STATE_TRACKING);
    assert(tracking_count == 1);
    assert(siren_off_count == 1);

    sm_process_event(&sm, EVT_DISARM_CMD, 20000);
    assert(sm_get_state(&sm) == STATE_DISARMED);

    printf("  ✓ full alert sequence\n");
}

void test_false_positive_returns_to_armed(void)
{
    alarm_sm_t sm;
    state_callbacks_t cbs = { .on_arm = cb_arm, .on_warning = cb_warning };
    sm_init(&sm, &cbs, 5000);
    reset_counts();

    sm_process_event(&sm, EVT_ARM_CMD, 0);
    sm_set_trigger(&sm, TRIGGER_PROXIMITY);
    sm_process_event(&sm, EVT_SENSOR_TRIGGER, 1000);
    assert(sm_get_state(&sm) == STATE_WARNING);

    sm_process_event(&sm, EVT_FALSE_POSITIVE, 3000);
    assert(sm_get_state(&sm) == STATE_ARMED);

    printf("  ✓ false positive returns to armed\n");
}

void test_disarm_from_any_state(void)
{
    alarm_sm_t sm;
    state_callbacks_t cbs = { .on_arm = cb_arm, .on_disarm = cb_disarm,
                             .on_warning = cb_warning, .on_alert = cb_alert,
                             .on_siren_on = cb_siren_on, .on_siren_off = cb_siren_off };
    sm_init(&sm, &cbs, 5000);

    // Disarm from WARNING
    sm_process_event(&sm, EVT_ARM_CMD, 0);
    sm_process_event(&sm, EVT_SENSOR_TRIGGER, 1000);
    assert(sm_get_state(&sm) == STATE_WARNING);
    sm_process_event(&sm, EVT_DISARM_CMD, 2000);
    assert(sm_get_state(&sm) == STATE_DISARMED);

    // Disarm from ALERT
    sm_process_event(&sm, EVT_ARM_CMD, 3000);
    sm_process_event(&sm, EVT_SENSOR_TRIGGER, 4000);
    sm_process_event(&sm, EVT_THREAT_CONFIRMED, 5000);
    assert(sm_get_state(&sm) == STATE_ALERT);
    sm_process_event(&sm, EVT_DISARM_CMD, 6000);
    assert(sm_get_state(&sm) == STATE_DISARMED);

    printf("  ✓ disarm from any state\n");
}

void test_ignore_invalid_events(void)
{
    alarm_sm_t sm;
    sm_init(&sm, NULL, 5000);

    // Sensor trigger while disarmed should be ignored
    sm_process_event(&sm, EVT_SENSOR_TRIGGER, 1000);
    assert(sm_get_state(&sm) == STATE_DISARMED);

    // GPS moving while armed should be ignored
    sm_process_event(&sm, EVT_ARM_CMD, 2000);
    sm_process_event(&sm, EVT_GPS_MOVING, 3000);
    assert(sm_get_state(&sm) == STATE_ARMED);

    printf("  ✓ ignore invalid events\n");
}

int main(void)
{
    printf("State machine tests:\n");
    test_basic_arm_disarm();
    test_full_alert_sequence();
    test_false_positive_returns_to_armed();
    test_disarm_from_any_state();
    test_ignore_invalid_events();
    printf("\nAll state machine tests passed! ✓\n");
    return 0;
}
