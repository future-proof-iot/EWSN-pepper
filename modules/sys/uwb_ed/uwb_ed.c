/*
 * Copyright (C) 2021 Inria
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @ingroup     module_uwb_ed
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
#include "uwb_ed.h"
#ifndef LOG_LEVEL
#define LOG_LEVEL   LOG_INFO
#endif
#include "log.h"

void uwb_ed_add(uwb_ed_list_t *list, uwb_ed_t *uwb_ed)
{
    assert(list && uwb_ed);

    unsigned state = irq_disable();
    if (!uwb_ed->list_node.next) {
        clist_rpush(&list->list, &uwb_ed->list_node);
    }
    irq_restore(state);
}

void uwb_ed_remove(uwb_ed_list_t *list, uwb_ed_t *uwb_ed)
{
    assert(list && uwb_ed);

    unsigned state = irq_disable();
    clist_remove(&list->list, &uwb_ed->list_node);
    uwb_ed->list_node.next = NULL;
    irq_restore(state);
}

uwb_ed_t *uwb_ed_list_get_by_short_addr(uwb_ed_list_t *list,
                                        const uint16_t addr)
{
    uwb_ed_t *tmp = (uwb_ed_t *)list->list.next;
    unsigned state = irq_disable();

    if (!tmp) {
        irq_restore(state);
        return NULL;
    }
    do {
        tmp = (uwb_ed_t *)tmp->list_node.next;
        if ((uint16_t)tmp->cid == addr) {
            irq_restore(state);
            return tmp;
        }
    } while (tmp != (uwb_ed_t *)list->list.next);
    irq_restore(state);
    return NULL;
}

uwb_ed_t *uwb_ed_list_get_by_cid(uwb_ed_list_t *list, const uint32_t cid)
{
    uwb_ed_t *tmp = (uwb_ed_t *)list->list.next;
    unsigned state = irq_disable();

    if (!tmp) {
        irq_restore(state);
        return NULL;
    }
    do {
        tmp = (uwb_ed_t *)tmp->list_node.next;
        if (tmp->cid == cid) {
            irq_restore(state);
            return tmp;
        }
    } while (tmp != (uwb_ed_t *)list->list.next);
    irq_restore(state);
    return NULL;
}

uint32_t uwb_ed_exposure_time(uwb_ed_t *uwb_ed)
{
    return uwb_ed->seen_last_s - uwb_ed->seen_first_s;
}

int uwb_ed_add_slice(uwb_ed_t *uwb_ed, uint16_t time, const uint8_t *slice,
                     uint8_t part)
{
    if (uwb_ed->ebid.status.status != EBID_HAS_ALL) {
        if (part == EBID_SLICE_3) {
            /* DESIRE sends sends the third slice with front padding so
               ignore first 4 bytes:
               https://gitlab.inria.fr/aboutet1/test-bluetooth/-/blob/master/app/src/main/java/fr/inria/desire/ble/models/AdvPayload.kt#L54
             */
            slice += EBID_SLICE_SIZE_PAD;
        }
        ebid_set_slice(&uwb_ed->ebid, slice, part);
        if (ebid_reconstruct(&uwb_ed->ebid) == 0) {
            uwb_ed->seen_first_s = time;
            uwb_ed->seen_last_s = time;
            LOG_INFO("[uwb_ed]: saw ebid: [");
            for (uint8_t i = 0; i < EBID_SIZE; i++) {
                LOG_INFO("%d, ", uwb_ed->ebid.parts.ebid.u8[i]);
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

void uwb_ed_process_data(uwb_ed_t *uwb_ed, uint16_t time, uint16_t d_cm)
{
    if (time > uwb_ed->seen_last_s) {
        uwb_ed->seen_last_s = time;
    }
    uwb_ed->req_count++;
    uwb_ed->cumulative_d_cm += d_cm;
}

uwb_ed_t *uwb_ed_list_process_slice(uwb_ed_list_t *list, const uint32_t cid,
                                    uint16_t time, const uint8_t *slice,
                                    uint8_t part)
{
    uwb_ed_t *uwb_ed = uwb_ed_list_get_by_cid(list, cid);

    if (!uwb_ed) {
        LOG_DEBUG("[uwb_ed]: ble_addr not found in list\n");
        uwb_ed = uwb_ed_memory_manager_calloc(list->manager);
        uwb_ed_init(uwb_ed, cid);
        if (!uwb_ed) {
            LOG_ERROR("[uwb_ed]: no memory to allocate new uwb_ed struct\n");
            return NULL;
        }
        uwb_ed_add(list, uwb_ed);
    }
    uwb_ed_add_slice(uwb_ed, time, slice, part);

    return uwb_ed;
}

void uwb_ed_list_process_rng_data(uwb_ed_list_t *list, const uint16_t addr,
                                  uint16_t time, uint16_t d_cm)
{
    uwb_ed_t *uwb_ed = uwb_ed_list_get_by_short_addr(list, addr);

    if (!uwb_ed) {
        LOG_ERROR("[uwb_ed]: could not find by addr\n");
    }
    if (uwb_ed->ebid.status.status == EBID_HAS_ALL) {
        uwb_ed_process_data(uwb_ed, time, d_cm);
    }
}

int uwb_ed_finish(uwb_ed_t *uwb_ed)
{
    /* if exposure time was enough then the ebid must have been
       reconstructed */
    if (uwb_ed_exposure_time(uwb_ed) >= MIN_EXPOSURE_TIME_S) {
        if (uwb_ed->req_count > 0) {
            uwb_ed->cumulative_d_cm = uwb_ed->cumulative_d_cm /
                                      uwb_ed->req_count;
        }
        return 0;
    }
    else {
        LOG_DEBUG("[uwb_ed]: not enough exposure %" PRIu32 " %" PRIu32 "\n",
                  uwb_ed->seen_last_s, uwb_ed->seen_first_s);
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

void uwb_ed_list_finish(uwb_ed_list_t *list)
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
            if (uwb_ed_finish((uwb_ed_t *)node)) {
                LOG_DEBUG("[uwb_ed]: discarding node\n");
                /* if unable to reconstruct uwb_ed, or if exposure time is not enough then
                   discard encounter */
                unsigned state = irq_disable();
                _clist_remove_with_before(&list->list, node, before);
                irq_restore(state);
                uwb_ed_memory_manager_free(list->manager, (uwb_ed_t *)node);
                node = NULL;
            }
        } while (list->list.next && node != list->list.next);
    }
}

void uwb_ed_memory_manager_init(uwb_ed_memory_manager_t *manager)
{
    memset(manager, '\0', sizeof(uwb_ed_memory_manager_t));
    memarray_init(&manager->mem, manager->buf, sizeof(uwb_ed_t),
                  CONFIG_UWB_ED_BUF_SIZE);
}

void uwb_ed_memory_manager_free(uwb_ed_memory_manager_t *manager,
                                uwb_ed_t *uwb_ed)
{
    memarray_free(&manager->mem, uwb_ed);
}

uwb_ed_t *uwb_ed_memory_manager_calloc(uwb_ed_memory_manager_t *manager)
{
    return memarray_calloc(&manager->mem);
}
