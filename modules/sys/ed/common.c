/*
 * Copyright (C) 2021 Inria
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @ingroup     module_ed
 * @{
 *
 * @file
 * @brief       Encounter Data List implementation
 *
 * @author      Francisco Molina <francois-xavier.molina@inria.fr>
 *
 * @}
 */

#include <assert.h>
#include <string.h>

#include "irq.h"
#include "ed.h"
#include "ed_shared.h"

#include "fmt.h"
#include "timex.h"
#include "board.h"

#ifndef LOG_LEVEL
#define LOG_LEVEL   LOG_WARNING
#endif
#include "log.h"

void ed_add(ed_list_t *list, ed_t *ed)
{
    assert(list && ed);

    unsigned state = irq_disable();

    if (!ed->list_node.next) {
        clist_rpush(&list->list, &ed->list_node);
    }
    irq_restore(state);
}

void ed_remove(ed_list_t *list, ed_t *ed)
{
    assert(list && ed);

    unsigned state = irq_disable();

    clist_remove(&list->list, &ed->list_node);
    ed->list_node.next = NULL;
    irq_restore(state);
}

ed_t *ed_list_get_nth(ed_list_t *list, int pos)
{
    ed_t *tmp = (ed_t *)clist_lpeek(&list->list);
    unsigned state = irq_disable();

    for (int i = 0; (i < pos) && tmp; i++) {
        tmp = (ed_t *)tmp->list_node.next;
    }
    irq_restore(state);
    return tmp;
}

ed_t *ed_list_get_by_short_addr(ed_list_t *list, const uint16_t addr)
{
    ed_t *tmp = (ed_t *)list->list.next;
    unsigned state = irq_disable();

    if (!tmp) {
        irq_restore(state);
        return NULL;
    }
    do {
        tmp = (ed_t *)tmp->list_node.next;
        if ((uint16_t)tmp->cid == addr) {
            irq_restore(state);
            return tmp;
        }
    } while (tmp != (ed_t *)list->list.next);
    irq_restore(state);
    return NULL;
}

ed_t *ed_list_get_by_cid(ed_list_t *list, const uint32_t cid)
{
    ed_t *tmp = (ed_t *)list->list.next;
    unsigned state = irq_disable();

    if (!tmp) {
        irq_restore(state);
        return NULL;
    }
    do {
        tmp = (ed_t *)tmp->list_node.next;
        if (tmp->cid == cid) {
            irq_restore(state);
            return tmp;
        }
    } while (tmp != (ed_t *)list->list.next);
    irq_restore(state);
    return NULL;
}

int ed_add_slice(ed_t *ed, uint16_t time, const uint8_t *slice, uint8_t part)
{
    if (ed->ebid.status.status != EBID_HAS_ALL) {
        if (part == EBID_SLICE_3) {
            /* DESIRE sends sends the third slice with front padding so
               ignore first 4 bytes:
               https://gitlab.inria.fr/aboutet1/test-bluetooth/-/blob/master/app/src/main/java/fr/inria/desire/ble/models/AdvPayload.kt#L54
             */
            slice += EBID_SLICE_SIZE_LONG - EBID_SLICE_SIZE_SHORT;
        }
        ebid_set_slice(&ed->ebid, slice, part);
        if (ebid_reconstruct(&ed->ebid) == 0) {
            #if IS_USED(MODULE_ED_BLE)
            ed->ble.seen_first_s = time;
            ed->ble.seen_last_s = time;
            #endif
            #if IS_USED(MODULE_ED_BLE_WIN)
            ed->ble_win.seen_first_s = time;
            ed->ble_win.seen_last_s = time;
            #endif
            #if IS_USED(MODULE_ED_UWB)
            ed->uwb.seen_first_s = time;
            /* for uwb seen_last_s is set after first successful TWR */
            #endif
            LOG_WARNING("[ed]: encountered 0x%04" PRIx16 " at t=(%" PRIu16 "s)\n",
                        ed_get_short_addr(ed), time);
            LOG_INFO("\n\tEBID: ");
            for (uint8_t i = 0; i < EBID_SIZE; i++) {
                if ((i + 1) % 8 == 0 && i != (EBID_SIZE - 1)) {
                    LOG_INFO("0x%02x\n\t", ed->ebid.parts.ebid.u8[i]);
                }
                else {
                    LOG_INFO("0x%02x ", ed->ebid.parts.ebid.u8[i]);
                }
            }
            LOG_INFO("\n");
            return 0;
        }
        return -1;
    }
    else {
        return 1;
    }
}

ed_t *ed_list_process_slice(ed_list_t *list, const uint32_t cid, uint16_t time,
                            const uint8_t *slice, uint8_t part)
{
    ed_t *ed = ed_list_get_by_cid(list, cid);

    if (!ed) {
        LOG_DEBUG("[ed]: cid not found in list\n");
        ed = ed_memory_manager_calloc(list->manager);
        if (!ed) {
            LOG_WARNING("[ed]: no memory to allocate new ed struct\n");
            return NULL;
        }
        ed_init(ed, cid);
        ed_add(list, ed);
    }
    if (ed_add_slice(ed, time, slice, part) == 0) {
#if IS_USED(MODULE_ED_BLE_WIN) || IS_USED(MODULE_ED_BLE)
        ed_ble_set_obf_value(ed, list->ebid);
#endif
    }

    return ed;
}

bool ed_finish(ed_t *ed, uint32_t min_exposure_s)
{
    bool valid = false;

    (void)ed;
    (void)min_exposure_s;
#if IS_USED(MODULE_ED_BLE_COMMON)
    bool valid_ble = false;
#endif
#if IS_USED(MODULE_ED_BLE)
    valid_ble |= ed_ble_finish(ed, min_exposure_s);
    valid |= valid_ble;
#endif
#if IS_USED(MODULE_ED_BLE_WIN)
    valid_ble |= ed_ble_win_finish(ed, min_exposure_s);
    valid |= valid_ble;
#endif
#if IS_USED(MODULE_ED_UWB)
    bool valid_uwb = ed_uwb_finish(ed, min_exposure_s);
    valid |= valid_uwb;
#if IS_USED(MODULE_ED_BLE_COMMON) && IS_USED(MODULE_ED_LEDS)
    if (valid_uwb != valid_ble) {
        if (IS_ACTIVE(MODULE_ED_LEDS)) {
            ed_blink_start(LED1_PIN, 10 * MS_PER_SEC);
        }
    }
#endif
#endif
    return valid;
}

/* since we will always know the before node this will update the clist more
   efficiently */
static void _clist_remove_with_before(clist_node_t *list, clist_node_t *node,
                                      clist_node_t *before)
{
    unsigned state = irq_disable();

    /* we are removing the last element in the list */
    if (before == node) {
        list->next = NULL;
    }
    /* there are at least two elements in the list */
    before->next = before->next->next;
    if (node == list->next) {
        list->next = before;
    }
    irq_restore(state);
}

void ed_list_finish(ed_list_t *list)
{
    clist_node_t *node = list->list.next;

    if (node) {
        do {
            clist_node_t *before = node;
            node = node->next;
            /* check if we are handling the last node (start of list)*/
            bool last_node = node == list->list.next;
            /* check if node should be kept */
            if (!ed_finish((ed_t *)node, list->min_exposure_s)) {
                LOG_DEBUG("[ed]: discarding node\n");
                /* remove the current node from list, and pass previous
                    node to easily update list */
                _clist_remove_with_before(&list->list, node, before);
                /* free up the resource */
                ed_memory_manager_free(list->manager, (ed_t *)node);
                /* reset node to previous iteration */
                node = before;
            }
            if (last_node) {
                return;
            }
        } while (list->list.next);
    }
}

void ed_list_clear(ed_list_t *list)
{
    unsigned state = irq_disable();

    while (list->list.next) {
        ed_memory_manager_free(list->manager, (ed_t *)clist_lpop(&list->list));
    }
    irq_restore(state);
}

void ed_memory_manager_init(ed_memory_manager_t *manager)
{
    memset(manager, '\0', sizeof(ed_memory_manager_t));
    memarray_init(&manager->mem, manager->buf, sizeof(ed_t), CONFIG_ED_BUF_SIZE);
}

void ed_memory_manager_free(ed_memory_manager_t *manager, ed_t *ed)
{
    memarray_free(&manager->mem, ed);
}

ed_t *ed_memory_manager_calloc(ed_memory_manager_t *manager)
{
    return memarray_calloc(&manager->mem);
}

size_t ed_serialize_uwb_ble_csv(ed_uwb_data_t *uwb, ed_ble_data_t *ble, const char *bn, char *buf)
{
    int size = 0;

    if (bn) {
        size += fmt_str(buf + size, bn);
        size += fmt_char(buf + size, ',');
    }
    if (uwb) {
        size += fmt_bytes_hex(buf + size, (uint8_t *)&uwb->cid, sizeof(uint32_t));
        size += fmt_char(buf + size, ',');
        size += fmt_u32_dec(buf + size, uwb->time);
        size += fmt_char(buf + size, ',');
    }
    else if (ble) {
        size += fmt_bytes_hex(buf + size, (uint8_t *)&ble->cid, sizeof(uint32_t));
        size += fmt_char(buf + size, ',');
        size += fmt_u32_dec(buf + size, ble->time);
        size += fmt_char(buf + size, ',');
    }

    if (uwb) {
        size += fmt_u32_dec(buf + size, uwb->d_cm);
        size += fmt_char(buf + size, ',');
#if IS_USED(MODULE_ED_UWB_LOS)
        size += fmt_u32_dec(buf + size, uwb->los);
#else
        size += fmt_str(buf + size, "NULL");
#endif
        size += fmt_char(buf + size, ',');
#if IS_USED(MODULE_ED_UWB_RSSI)
        size += fmt_float(buf + size, uwb->rssi, 2);
#else
        size += fmt_str(buf + size, "NULL");
#endif
        size += fmt_char(buf + size, ',');
    }
    else {
        size += fmt_str(buf + size, "NULL");
        size += fmt_char(buf + size, ',');
        size += fmt_str(buf + size, "NULL");
        size += fmt_char(buf + size, ',');
        size += fmt_str(buf + size, "NULL");
        size += fmt_char(buf + size, ',');
    }
    if (ble) {
        size += fmt_float(buf + size, ble->rssi, 2);
        size += fmt_char(buf + size, ',');
    }
    else {
        size += fmt_str(buf + size, "NULL");
        size += fmt_char(buf + size, ',');
    }

    return size;
}

void ed_serialize_uwb_ble_printf_csv(ed_uwb_data_t *uwb, ed_ble_data_t *ble, const char *bn)
{
    if (bn) {
        print_str(bn);
        print_str(",");
    }
    if (uwb) {
        print_bytes_hex((uint8_t *)&uwb->cid, sizeof(uint32_t));
        print_str(",");
        print_u32_dec(uwb->time);
        print_str(",");
    }
    else {
        print_bytes_hex((uint8_t *)&ble->cid, sizeof(uint32_t));
        print_str(",");
        print_u32_dec(ble->time);
        print_str(",");
    }
    if (uwb) {
        print_u32_dec(uwb->d_cm);
        print_str(",");
#if IS_USED(MODULE_ED_UWB_LOS)
        print_u32_dec(uwb->los);
#else
        print_str("NULL");
#endif
        print_str(",");
#if IS_USED(MODULE_ED_UWB_RSSI)
        print_float(uwb->rssi, 2);
#else
        print_str("NULL");
#endif
        print_str(",");
    }
    else {
        print_str("NULL");
        print_str(",");
        print_str("NULL");
        print_str(",");
        print_str("NULL");
        print_str(",");
    }
    if (ble) {
        print_float(ble->rssi, 2);
        print_str(",");
    }
    else {
        print_str("NULL");
        print_str(",");
    }
    print_str("\n");
}
