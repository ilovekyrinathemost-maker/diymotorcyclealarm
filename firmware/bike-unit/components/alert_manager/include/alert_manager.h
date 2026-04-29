#ifndef ALERT_MANAGER_H
#define ALERT_MANAGER_H

#include <stdint.h>
#include <stdbool.h>

// === States ===
typedef enum {
    STATE_INIT = 0,
    STATE_DISARMED,
    STATE_ARMED,
    STATE_WARNING,
    STATE_ALERT,
    STATE_TRACKING,
} alarm_state_t;

// === Events ===
typedef enum {
    EVT_ARM_CMD = 0,
    EVT_DISARM_CMD,
    EVT_SENSOR_TRIGGER,
    EVT_THREAT_CONFIRMED,
    EVT_FALSE_POSITIVE,
    EVT_GPS_MOVING,
    EVT_WARNING_TIMEOUT,
} alarm_event_t;

// === Trigger Source ===
typedef enum {
    TRIGGER_MOTION = 0,
    TRIGGER_TILT,
    TRIGGER_PROXIMITY,
    TRIGGER_IGNITION,
    TRIGGER_BATTERY_CUT,
    TRIGGER_GEOFENCE,
    TRIGGER_ENCLOSURE,
} trigger_source_t;

// === Callbacks (actions taken on transitions) ===
typedef struct {
    void (*on_arm)(void);
    void (*on_disarm)(void);
    void (*on_warning)(trigger_source_t source);
    void (*on_alert)(trigger_source_t source);
    void (*on_tracking)(void);
    void (*on_alert_cleared)(void);
    void (*on_siren_on)(void);
    void (*on_siren_off)(void);
} state_callbacks_t;

// === State Machine Context ===
typedef struct {
    alarm_state_t current_state;
    trigger_source_t last_trigger;
    uint32_t state_entered_ms;     // Timestamp when entered current state
    uint32_t warning_start_ms;     // When warning started
    uint32_t warning_timeout_ms;   // How long to verify (default 5000)
    state_callbacks_t callbacks;
} alarm_sm_t;

// === Functions ===
void sm_init(alarm_sm_t *sm, state_callbacks_t *callbacks, uint32_t warning_timeout_ms);
alarm_state_t sm_get_state(const alarm_sm_t *sm);
void sm_process_event(alarm_sm_t *sm, alarm_event_t event, uint32_t now_ms);
void sm_set_trigger(alarm_sm_t *sm, trigger_source_t source);
const char *sm_state_name(alarm_state_t state);

#endif // ALERT_MANAGER_H
