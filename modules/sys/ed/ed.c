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
#ifndef LOG_LEVEL
#define LOG_LEVEL   LOG_INFO
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

uint16_t ed_exposure_time(ed_t *ed)
{
    return ed->end_s - ed->start_s;
}

void ed_set_obf_value(ed_t *ed, ebid_t *ebid)
{
    bool local_gt_remote = false;

    /* compare local ebid and the remote one to see which one
       is greater */
    for (uint8_t i = 0; i < EBID_SIZE; i++) {
        if (ebid->parts.ebid.u8[i] > ed->ebid.parts.ebid.u8[i]) {
            local_gt_remote = true;
            break;
        }
        else if (ebid->parts.ebid.u8[i] < ed->ebid.parts.ebid.u8[i]) {
            break;
        }
    }
    /* calculate obf depending on which ebid is greater */
    if (local_gt_remote) {
        ed->obf = (ebid->parts.ebid.u8[0] << 8) | ebid->parts.ebid.u8[1];
    }
    else {
        ed->obf = (ed->ebid.parts.ebid.u8[0] << 8) | ed->ebid.parts.ebid.u8[1];
    }
    ed->obf %= CONFIG_ED_OBFUSCATE_MAX;
}

int ed_add_slice(ed_t *ed, uint16_t time, const uint8_t *slice, uint8_t part,
                 ebid_t *ebid_local)
{
    if (ed->ebid.status.status != EBID_HAS_ALL) {
        if (part == EBID_SLICE_3) {
            /* DESIRE sends sends the third slice with front padding so
               ignore first 4 bytes:
               https://gitlab.inria.fr/aboutet1/test-bluetooth/-/blob/master/app/src/main/java/fr/inria/desire/ble/models/AdvPayload.kt#L54
             */
            slice += EBID_SLICE_SIZE_PAD;
        }
        ebid_set_slice(&ed->ebid, slice, part);
        if (ebid_reconstruct(&ed->ebid) == 0) {
            ed_set_obf_value(ed, ebid_local);
            ed->start_s = time;
            LOG_INFO("[ed]: saw ebid: [");
            for (uint8_t i = 0; i < EBID_SIZE; i++) {
                LOG_INFO("%d, ", ed->ebid.parts.ebid.u8[i]);
            }
            LOG_INFO("]\n");
            return 0;
        }
        return -1;
    }
    else {
        return 0;
    }
}

void ed_process_data(ed_t *ed, uint16_t time, float rssi)
{
    if (time > ed->end_s) {
        ed->end_s = time;
    }
    if (IS_ACTIVE(CONFIG_ED_OBFUSCATE_RSSI)) {
        /* obfuscate rssi value */
        rssi = rssi - ed->obf - CONFIG_RX_COMPENSATION_GAIN;
    }
    rdl_windows_update(&ed->wins, rssi, time);
}

int ed_list_process_data(ed_list_t *list, const uint32_t cid, uint16_t time,
                         const uint8_t *slice, uint8_t part, float rssi)
{
    ed_t *ed = ed_list_get_by_cid(list, cid);

    if (!ed) {
        LOG_DEBUG("[ed]: ble_addr not found in list\n");
        ed = ed_memory_manager_calloc(list->manager);
        ed_init(ed, cid);
        if (!ed) {
            LOG_ERROR("[ed]: no memory to allocate new ed struct\n");
            return -1;
        }
        ed_add(list, ed);
    }
    /* only add data once ebid was reconstructed */
    if (ed_add_slice(ed, time, slice, part, list->ebid) == 0) {
        ed_process_data(ed, time, rssi);
    }
    return 0;
}
int ed_finish(ed_t *ed)
{
    /* if exposure time was enough then the ebid must have been
       reconstructed */
    if (ed_exposure_time(ed) >= MIN_EXPOSURE_TIME_S) {
        rdl_windows_finalize(&ed->wins);
        return 0;
    }

    return -1;
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
            clist_node_t* before = node;
            node = node->next;
            /* check if we are handling the last node (start of list)*/
            bool last_node = node == list->list.next;
            if (ed_finish((ed_t *)node)) {
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
