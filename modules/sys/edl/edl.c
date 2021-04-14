/*
 * Copyright (C) 2021 Inria
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @ingroup     module_edl
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

#include "edl.h"

void edl_add(edl_list_t *list, edl_t* edl)
{
    assert(list && edl);

    unsigned state = irq_disable();
    if (!edl->list_node.next) {
        clist_rpush(&list->list, &edl->list_node);
    }
    irq_restore(state);
}

void edl_remove(edl_list_t *list, edl_t* edl)
{
    assert(list && edl);

    unsigned state = irq_disable();
    clist_remove(&list->list, &edl->list_node);
    edl->list_node.next = NULL;
    irq_restore(state);
}

edl_t* edl_list_get_nth(edl_list_t *list, int pos)
{
    edl_t *tmp = (edl_t *) clist_lpeek(&list->list);
    unsigned state = irq_disable();
    for (int i = 0; (i < pos) && tmp; i++) {
        tmp = (edl_t *) tmp->list_node.next;
    }
    irq_restore(state);
    return tmp;
}

edl_t* edl_list_get_before(edl_list_t *list, uint32_t before)
{
    edl_t *tmp = (edl_t *) clist_lpeek(&list->list);
    unsigned state = irq_disable();
    if (!tmp) {
        irq_restore(state);
        return NULL;
    }
    do {
        tmp = (edl_t *) tmp->list_node.next;
        if (tmp->timestamp < before) {
            irq_restore(state);
            return tmp;
        }
    } while( tmp != (edl_t *) list->list.next );
    irq_restore(state);
    return NULL;
}

edl_t* edl_list_get_after(edl_list_t *list, uint32_t after)
{
    edl_t *tmp = (edl_t *) clist_lpeek(&list->list);
    unsigned state = irq_disable();
    if (!tmp) {
        irq_restore(state);
        return NULL;
    }
    do {
        tmp = (edl_t *) tmp->list_node.next;
        if (tmp->timestamp > after) {
            irq_restore(state);
            return tmp;
        }
    } while( tmp != (edl_t *) list->list.next );
    irq_restore(state);
    return NULL;
}

edl_t* edl_list_get_by_time(edl_list_t *list, uint32_t time)
{
    edl_t *tmp = (edl_t *) clist_lpeek(&list->list);
    unsigned state = irq_disable();
    if (!tmp) {
        irq_restore(state);
        return NULL;
    }
    do {
        tmp = (edl_t *) tmp->list_node.next;
        if (tmp->timestamp == time) {
            irq_restore(state);
            return tmp;
        }
    } while( tmp != (edl_t *) list->list.next );
    irq_restore(state);
    return NULL;
}

uint32_t edl_list_exposure_time(edl_list_t *list)
{
    if (!&list->list.next) {
        return 0;
    }
    edl_t *first = (edl_t*) clist_rpeek(&list->list);
    edl_t *last = (edl_t*) clist_lpeek(&list->list);
    assert(first->timestamp >= last->timestamp);
    return first->timestamp - last->timestamp;
}
