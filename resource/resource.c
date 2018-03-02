/*
 * This file is part of SID.
 *
 * Copyright (C) 2017-2018 Red Hat, Inc. All rights reserved.
 *
 * SID is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * SID is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with SID.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <systemd/sd-daemon.h>
#include <unistd.h>
#include "list.h"
#include "log.h"
#include "mem.h"
#include "resource.h"

struct sid_resource {
	struct list list;
	const struct sid_resource_reg *reg;
	char *id;
	struct sid_resource *parent;
	struct list children;
	sd_event *event_loop;
	pid_t pid_created;
	void *data;
};

struct sid_resource_iter {
	struct sid_resource *res;
	struct list *prev; /* for safety */
	struct list *current;
	struct list *next; /* for safety */
};

struct sid_resource *sid_resource_create(struct sid_resource *parent_res, const struct sid_resource_reg *reg,
					 uint64_t flags __attribute__((unused)), const char *id_part, const void *kickstart_data)
{
	struct sid_resource *res;
	size_t id_size;
	char *id;
	struct sid_resource *child_res, *tmp_child_res;

	/* +1 for '/' if id is defined and +1 for '\0' at the end */
	id_size = (reg->name ? strlen(reg->name) : 0) + (id_part ? strlen(id_part) + 1 : 0) + 1;

	if (!(id = malloc(id_size)))
		goto fail;

	if (snprintf(id, id_size, "%s%s%s", reg->name ? : "", id_part ? "/" : "", id_part ? : "") < 0)
		goto fail;

	log_debug(id, "Creating resource.");

	if (!(res = zalloc(sizeof(*res))))
		goto fail;

	res->id = id;
	list_init(&res->children);
	res->reg = reg;
	res->event_loop = NULL;
	res->pid_created = getpid(); /* FIXME: Use cached pid instead? Check latency... */

	if (reg->with_event_loop && sd_event_new(&res->event_loop) < 0)
		goto fail;

	if ((res->parent = parent_res))
		list_add(&parent_res->children, &res->list);

	if (reg->with_event_loop && reg->with_watchdog &&
	    sd_event_set_watchdog(res->event_loop, 1) < 0)
		goto fail;

	if (reg->init && reg->init(res, kickstart_data, &res->data) < 0)
		goto fail;

	log_debug(res->id, "Resource created.");
	return res;
fail:
	if (res) {
		list_iterate_items_safe_back(child_res, tmp_child_res, &res->children)
			(void) sid_resource_destroy(child_res);
		if (res->parent)
			list_del(&res->list);
		if (res->event_loop)
			sd_event_unref(res->event_loop);
		free(res);
	}

	log_debug(id, "Resource NOT created.");
	free(id);
	return NULL;
}

int sid_resource_destroy(struct sid_resource *res)
{
	static const char msg_destroying[] = "Destroying resource";
	static const char msg_destroyed[] = "Resource destroyed";
	static const char msg_pid_created_current[] = "PID created/current";
	struct sid_resource *child_res, *tmp_child_res;
	pid_t pid = getpid();

	if (pid == res->pid_created)
		log_debug(res->id, "%s.", msg_destroying);
	else
		log_debug(res->id, "%s (%s: %d/%d).", msg_destroying,
			  msg_pid_created_current, res->pid_created, pid);

	list_iterate_items_safe_back(child_res, tmp_child_res, &res->children)
		(void) sid_resource_destroy(child_res);

	if (res->reg->destroy)
		(void) res->reg->destroy(res);

	if (res->event_loop)
		res->event_loop = sd_event_unref(res->event_loop);

	if (res->parent)
		list_del(&res->list);

	if (pid == res->pid_created)
		log_debug(res->id, "%s.", msg_destroyed);
	else
		log_debug(res->id, "%s (%s: %d/%d).", msg_destroyed,
			  msg_pid_created_current, res->pid_created, pid);

	free(res->id);
	free(res);

	return 0;
}

bool sid_resource_is_registered_by(struct sid_resource *res, const struct sid_resource_reg *reg)
{
	return res->reg == reg;
}

const char *sid_resource_get_id(struct sid_resource *res)
{
	return res->id;
}

void *sid_resource_get_data(struct sid_resource *res)
{
	return res->data;
}

struct sid_resource *_get_resource_with_event_loop(struct sid_resource *res, int error_if_not_found)
{
	struct sid_resource *tmp_res = res;

	do {
		if (tmp_res->event_loop)
			return tmp_res;
		tmp_res = tmp_res->parent;
	} while (tmp_res);

	if (error_if_not_found)
		log_error(res->id, INTERNAL_ERROR "No event loop found.");

	return NULL;
}

int sid_resource_create_io_event_source(struct sid_resource *res, sid_event_source **es, int fd,
				        sid_io_handler handler, const char *name, void *data)
{
	struct sid_resource *res_event_loop = _get_resource_with_event_loop(res, 1);
	int r;

	if (!res_event_loop)
		return -ENOMEDIUM;

	r = sd_event_add_io(res_event_loop->event_loop, es, fd, EPOLLIN, handler, data);
	if (r < 0)
		return r;

	if (name)
		(void) sd_event_source_set_description(*es, name);

	return 0;
}

int sid_resource_create_signal_event_source(struct sid_resource *res, sid_event_source **es, int signal,
					    sid_signal_handler handler, const char *name, void *data)
{
	struct sid_resource *res_event_loop = _get_resource_with_event_loop(res, 1);
	int r;

	if (!res_event_loop)
		return -ENOMEDIUM;

	r =sd_event_add_signal(res_event_loop->event_loop, es, signal, handler, data);
	if (r < 0)
		return r;

	if (name)
		(void) sd_event_source_set_description(*es, name);

	return 0;
}

int sid_resource_create_child_event_source(struct sid_resource *res, sid_event_source **es, pid_t pid,
					   int options, sid_child_handler handler, const char *name, void *data)
{
	struct sid_resource *res_event_loop = _get_resource_with_event_loop(res, 1);
	int r;

	if (!res_event_loop)
		return -ENOMEDIUM;

	r = sd_event_add_child(res_event_loop->event_loop, es, pid, options, handler, data);
	if (r < 0)
		return r;

	if (name)
		(void) sd_event_source_set_description(*es, name);

	return 0;
}

int sid_resource_create_time_event_source(struct sid_resource *res, sid_event_source **es, clockid_t clock,
					  uint64_t usec, uint64_t accuracy, sid_time_handler handler,
					  const char *name, void *data)
{
	struct sid_resource *res_event_loop = _get_resource_with_event_loop(res, 1);
	int r;

	if (!res_event_loop)
		return -ENOMEDIUM;

	r = sd_event_add_time(res_event_loop->event_loop, es, clock, usec, accuracy, handler, data);
	if (r < 0)
		return r;

	if (name)
		(void) sd_event_source_set_description(*es, name);

	return 0;
}

int sid_resource_create_deferred_event_source(struct sid_resource *res, sid_event_source **es, sid_generic_handler handler, void *data)
{
	struct sid_resource *res_event_loop = _get_resource_with_event_loop(res, 1);

	if (!res_event_loop)
		return -ENOMEDIUM;

	return sd_event_add_defer(res_event_loop->event_loop, es, handler, data);
}

int sid_resource_create_post_event_source(struct sid_resource *res, sid_event_source **es, sid_generic_handler handler, void *data)
{
	struct sid_resource *res_event_loop = _get_resource_with_event_loop(res, 1);

	if (!res_event_loop)
		return -ENOMEDIUM;

	return sd_event_add_defer(res_event_loop->event_loop, es, handler, data);
}

int sid_resource_create_exit_event_source(struct sid_resource *res, sid_event_source **es, sid_generic_handler handler, void *data)
{
	struct sid_resource *res_event_loop = _get_resource_with_event_loop(res, 1);

	if (!res_event_loop)
		return -ENOMEDIUM;

	return sd_event_add_defer(res_event_loop->event_loop, es, handler, data);
}

int sid_resource_destroy_event_source(struct sid_resource *res __attribute__((unused)),
				      sid_event_source **es)
{
	*es = sd_event_source_unref(*es);
	return 0;
}

struct sid_resource *sid_resource_get_parent(struct sid_resource *res)
{
	return res->parent;
}

struct sid_resource *sid_resource_get_top_level(struct sid_resource *res)
{
	while (res->parent)
		res = res->parent;

	return res;
}

struct sid_resource *sid_resource_get_child(struct sid_resource *res, const struct sid_resource_reg *reg, const char *id)
{
	struct sid_resource *child_res;
	size_t id_offset = reg->name ? strlen(reg->name) + 1 : 0;

	list_iterate_items(child_res, &res->children) {
		if (child_res->reg == reg && !strcmp(child_res->id + id_offset, id))
			return child_res;
	}

	return NULL;
}

int sid_resource_add_child(struct sid_resource *res, struct sid_resource *child)
{
	if (!res || child->parent)
		return -EINVAL;

	child->parent = res;
	list_add(&res->children, &child->list);

	log_debug(res->id, "Child %s added.", child->id);
	return 0;
}

int sid_resource_isolate(struct sid_resource *res)
{
	struct sid_resource *tmp_child_res, *child_res;

	/* Only allow to isolate resource with parent and without event loop! */
	if (res->event_loop || !res->parent)
		return -EPERM;

	/* Reparent and isolate. */
	list_iterate_items_safe(child_res, tmp_child_res, &res->children) {
		list_del(&child_res->list);
		list_add(&res->parent->children, &child_res->list);
	}
	list_del(&res->list);
	res->parent = NULL;

	return 0;
}

int sid_resource_isolate_with_children(struct sid_resource *res)
{

	if (res->event_loop || !res->parent)
		return -EPERM;

	list_del(&res->list);
	res->parent = NULL;

	return 0;
}

struct sid_resource_iter *sid_resource_iter_create(struct sid_resource *res)
{
	struct sid_resource_iter *iter;

	if (!(iter = malloc(sizeof(*iter))))
		return NULL;

	iter->res = res;

	iter->current = &res->children;
	iter->prev = iter->current->p;
	iter->next = iter->current->n;

	return iter;
}

struct sid_resource *sid_resource_iter_current(struct sid_resource_iter *iter)
{
	if (iter->current == &iter->res->children)
		return NULL;

	return list_struct_base(iter->current, struct sid_resource, list);
}

struct sid_resource *sid_resource_iter_next(struct sid_resource_iter *iter)
{
	if (iter->next == &iter->res->children)
		return NULL;

	iter->current = iter->next;
	iter->next = iter->current->n;

	return list_struct_base(iter->current, struct sid_resource, list);
}

struct sid_resource *sid_resource_iter_previous(struct sid_resource_iter *iter)
{
	if (iter->prev == &iter->res->children)
		return NULL;

	iter->current = iter->prev;
	iter->prev = iter->current->p;

	return list_struct_base(iter->current, struct sid_resource, list);
}

void sid_resource_iter_reset(struct sid_resource_iter *iter)
{
	iter->current = &iter->res->children;
	iter->prev = iter->current->p;
	iter->next = iter->current->n;
}

void sid_resource_iter_destroy(struct sid_resource_iter *iter)
{
	free(iter);
}

int sid_resource_run_event_loop(struct sid_resource *res)
{
	int r;

	if (!res->event_loop)
		return -ENOMEDIUM;

	log_debug(res->id, "Entering event loop.");

	(void) sd_notify(0, "READY=1\n"
			    "STATUS=SID started processing requests.");

	r = sd_event_loop(res->event_loop);
	if (r < 0) {
		log_error_errno(res->id, -r, "Event loop failed.");
		return r;
	}

	log_debug(res->id, "Exiting event loop.");
	return 0;
}

int sid_resource_exit_event_loop(struct sid_resource *res)
{
	if (!res->event_loop)
		return -ENOMEDIUM;

	return sd_event_exit(res->event_loop, 0);
}