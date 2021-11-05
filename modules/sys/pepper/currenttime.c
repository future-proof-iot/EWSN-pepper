
#include "current_time.h"

#ifndef CONFIG_EPOCH_MAX_TIME_OFFSET
#define CONFIG_EPOCH_MAX_TIME_OFFSET            (CONFIG_EBID_ROTATION_T_S / 10)
#endif

static bool _epoch_reset = false;
static void _pre_adjust_time(int32_t offset, void *arg)
{
    (void)arg;
    _epoch_reset = offset > 0 ? offset > (int32_t) CONFIG_EPOCH_MAX_TIME_OFFSET :
                   -offset > (int32_t) CONFIG_EPOCH_MAX_TIME_OFFSET;
    if (_epoch_reset) {
        LOG_INFO("[pepper]: time was set back too much, bootstrap from 0\n");
        event_timeout_clear(&_silent_timeout);
        event_periodic_stop(&uwb_epoch_end);
        desire_ble_adv_stop();
        desire_ble_scan_stop();
    }
}
static current_time_hook_t _pre_hook;
static void _post_adjust_time(int32_t offset, void *arg)
{
    (void)offset;
    bool *epoch_restart = (bool *)arg;
    if (*epoch_restart) {
        _aligned_epoch_start();
        *epoch_restart = false;
    }
}
static current_time_hook_t _post_hook;


void pepper_current_time_init(void)
{
    current_time_init();
    current_time_hook_init(&_pre_hook, _pre_adjust_time, &_epoch_reset);
    current_time_hook_init(&_post_hook, _post_adjust_time, &_epoch_reset);
    current_time_add_pre_cb(&_pre_hook);
    current_time_add_post_cb(&_post_hook);
}
