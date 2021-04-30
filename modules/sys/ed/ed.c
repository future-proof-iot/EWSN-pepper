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

ed_t *ed_list_get_by_ble_addr(ed_list_t *list, const uint8_t *ble_addr)
{
    ed_t *tmp = (ed_t *)list->list.next;
    unsigned state = irq_disable();

    if (!tmp) {
        irq_restore(state);
        return NULL;
    }
    do {
        tmp = (ed_t *)tmp->list_node.next;
        if (memcmp(tmp->ble_addr, ble_addr, BLE_ADV_ADDR_SIZE) == 0) {
            irq_restore(state);
            return tmp;
        }
    } while (tmp != (ed_t *)list->list.next);
    irq_restore(state);
    return NULL;
}

uint16_t ed_exposure_time(ed_t *ed)
{
    return ed->end_s - ed->start_s;
}

void ed_process_data(ed_t *ed, uint16_t time, const uint8_t *slice,
                     uint8_t part,
                     float rssi)
{
    if (time > ed->end_s) {
        ed->end_s = time;
    }
    ebid_set_slice(&ed->ebid, slice, part);
    rdl_windows_update(&ed->wins, rssi, time);
}

int ed_list_process_data(ed_list_t *list, const uint8_t *ble_addr,
                         uint16_t time,
                         const uint8_t *slice, uint8_t part, float rssi)
{
    ed_t *ed = ed_list_get_by_ble_addr(list, ble_addr);

    if (!ed) {
        LOG_DEBUG("[ed]: ble_addr not found in list\n");
        ed = ed_memory_manager_calloc(list->manager);
        ed_init(ed, ble_addr);
        if (!ed) {
            LOG_ERROR("[ed]: no memory to allocate new ed struct\n");
            return -1;
        }
        ed_add(list, ed);
        ed->start_s = time;
    }
    ed_process_data(ed, time, slice, part, rssi);
    return 0;
}
int ed_finish(ed_t *ed)
{
    if (ed_exposure_time(ed) >= MIN_EXPOSURE_TIME_S) {
        if (ebid_reconstruct(&ed->ebid) == 0) {
            rdl_windows_finalize(&ed->wins);
            return 0;
        }
        LOG_DEBUG("[ed]: cant reconstruct ebid\n");
    }
    else {
        LOG_DEBUG("[ed]: not enough exposure\n");
    }

    return -1;
}

/* since we will always know the before node this will update the clist more
   efficiently */
static void _clist_remove_with_before(clist_node_t *list, clist_node_t *node,
                                      clist_node_t *before)
{
    /* we are removing the last element in the list */
    if (before == node) {
        list->next = NULL;
    }
    /* there are at least two elements in the list */
    before->next = before->next->next;
    if (node == list->next) {
        list->next = before;
    }
}

void ed_list_finish(ed_list_t *list)
{
    clist_node_t *node = list->list.next;
    clist_node_t *before = NULL;

    if (node) {
        do {
            /* make sure the first node gets handled if the node just before
               last got removed*/
            if (!node) {
                node = before;
            }
            before = node;
            node = node->next;
            if (ed_finish((ed_t *)node)) {
                /* if unable to reconstruct ed, or if exposure time is not enough then
                   discard encounter */
                unsigned state = irq_disable();
                _clist_remove_with_before(&list->list, node, before);
                irq_restore(state);
                ed_memory_manager_free(list->manager, (ed_t *)node);
                node = NULL;
            }
        } while (list->list.next && node != list->list.next);
    }
}

void ed_memory_manager_init(ed_memory_manager_t *manager)
{
    memset(manager, '\0', sizeof(ed_memory_manager_t));
    memarray_init(&manager->mem, manager->buf, sizeof(ed_t),
                  CONFIG_ED_BUF_SIZE);
}

void ed_memory_manager_free(ed_memory_manager_t *manager, ed_t *ed)
{
    memarray_free(&manager->mem, ed);
}

ed_t *ed_memory_manager_calloc(ed_memory_manager_t *manager)
{
    return memarray_calloc(&manager->mem);
}
