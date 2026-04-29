#include "alert_manager.h"
#include <string.h>

void sm_init(alarm_sm_t *sm, state_callbacks_t *callbacks, uint32_t warning_timeout_ms)
{
    memset(sm, 0, sizeof(alarm_sm_t));
    sm->current_state = STATE_DISARMED;
    sm->warning_timeout_ms = warning_timeout_ms;
    if (callbacks) {
        sm->callbacks = *callbacks;
    }
}

alarm_state_t sm_get_state(const alarm_sm_t *sm)
{
    return sm->current_state;
}

void sm_set_trigger(alarm_sm_t *sm, trigger_source_t source)
{
    sm->last_trigger = source;
}

void sm_process_event(alarm_sm_t *sm, alarm_event_t event, uint32_t now_ms)
{
    switch (sm->current_state) {
        case STATE_INIT:
            // Always transition to DISARMED on init
            sm->current_state = STATE_DISARMED;
            sm->state_entered_ms = now_ms;
            break;

        case STATE_DISARMED:
            if (event == EVT_ARM_CMD) {
                sm->current_state = STATE_ARMED;
                sm->state_entered_ms = now_ms;
                if (sm->callbacks.on_arm) sm->callbacks.on_arm();
            }
            break;

        case STATE_ARMED:
            if (event == EVT_DISARM_CMD) {
                sm->current_state = STATE_DISARMED;
                sm->state_entered_ms = now_ms;
                if (sm->callbacks.on_disarm) sm->callbacks.on_disarm();
            } else if (event == EVT_SENSOR_TRIGGER) {
                sm->current_state = STATE_WARNING;
                sm->state_entered_ms = now_ms;
                sm->warning_start_ms = now_ms;
                if (sm->callbacks.on_warning) sm->callbacks.on_warning(sm->last_trigger);
            }
            break;

        case STATE_WARNING:
            if (event == EVT_DISARM_CMD) {
                sm->current_state = STATE_DISARMED;
                sm->state_entered_ms = now_ms;
                if (sm->callbacks.on_disarm) sm->callbacks.on_disarm();
            } else if (event == EVT_THREAT_CONFIRMED) {
                sm->current_state = STATE_ALERT;
                sm->state_entered_ms = now_ms;
                if (sm->callbacks.on_alert) sm->callbacks.on_alert(sm->last_trigger);
                if (sm->callbacks.on_siren_on) sm->callbacks.on_siren_on();
            } else if (event == EVT_FALSE_POSITIVE || event == EVT_WARNING_TIMEOUT) {
                sm->current_state = STATE_ARMED;
                sm->state_entered_ms = now_ms;
                if (sm->callbacks.on_alert_cleared) sm->callbacks.on_alert_cleared();
            }
            break;

        case STATE_ALERT:
            if (event == EVT_DISARM_CMD) {
                sm->current_state = STATE_DISARMED;
                sm->state_entered_ms = now_ms;
                if (sm->callbacks.on_siren_off) sm->callbacks.on_siren_off();
                if (sm->callbacks.on_disarm) sm->callbacks.on_disarm();
            } else if (event == EVT_GPS_MOVING) {
                sm->current_state = STATE_TRACKING;
                sm->state_entered_ms = now_ms;
                if (sm->callbacks.on_siren_off) sm->callbacks.on_siren_off();
                if (sm->callbacks.on_tracking) sm->callbacks.on_tracking();
            }
            break;

        case STATE_TRACKING:
            if (event == EVT_DISARM_CMD) {
                sm->current_state = STATE_DISARMED;
                sm->state_entered_ms = now_ms;
                if (sm->callbacks.on_disarm) sm->callbacks.on_disarm();
            }
            break;
    }
}

const char *sm_state_name(alarm_state_t state)
{
    switch (state) {
        case STATE_INIT:      return "INIT";
        case STATE_DISARMED:  return "DISARMED";
        case STATE_ARMED:     return "ARMED";
        case STATE_WARNING:   return "WARNING";
        case STATE_ALERT:     return "ALERT";
        case STATE_TRACKING:  return "TRACKING";
        default:              return "UNKNOWN";
    }
}
