/*
 * This file is part of SID.
 *
 * Copyright (C) 2017-2020 Red Hat, Inc. All rights reserved.
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

#ifndef _SID_UCMD_MODULE_H
#define _SID_UCMD_MODULE_H

#include "internal/comp-attrs.h"

#include "internal/common.h"

#include "resource/module.h"

#include <inttypes.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define SID_UCMD_BLOCK_MOD_DIR                   LIBDIR "/" PACKAGE "/modules/ucmd/block"
#define SID_UCMD_TYPE_MOD_DIR                    LIBDIR "/" PACKAGE "/modules/ucmd/type"

#define SID_UCMD_MOD_FN_NAME_SCAN_A_INIT         "sid_ucmd_scan_a_init"
#define SID_UCMD_MOD_FN_NAME_SCAN_PRE            "sid_ucmd_scan_pre"
#define SID_UCMD_MOD_FN_NAME_SCAN_CURRENT        "sid_ucmd_scan_current"
#define SID_UCMD_MOD_FN_NAME_SCAN_NEXT           "sid_ucmd_scan_next"
#define SID_UCMD_MOD_FN_NAME_SCAN_POST_CURRENT   "sid_ucmd_scan_post_current"
#define SID_UCMD_MOD_FN_NAME_SCAN_POST_NEXT      "sid_ucmd_scan_post_next"
#define SID_UCMD_MOD_FN_NAME_SCAN_A_EXIT         "sid_ucmd_scan_a_exit"
#define SID_UCMD_MOD_FN_NAME_SCAN_REMOVE_INIT    "sid_ucmd_scan_remove_init"
#define SID_UCMD_MOD_FN_NAME_SCAN_REMOVE         "sid_ucmd_scan_remove"
#define SID_UCMD_MOD_FN_NAME_SCAN_REMOVE_EXIT    "sid_ucmd_scan_remove_exit"
#define SID_UCMD_MOD_FN_NAME_SCAN_B_INIT         "sid_ucmd_scan_b_init"
#define SID_UCMD_MOD_FN_NAME_SCAN_ACTION_CURRENT "sid_ucmd_scan_action_current"
#define SID_UCMD_MOD_FN_NAME_SCAN_ACTION_NEXT    "sid_ucmd_scan_action_next"
#define SID_UCMD_MOD_FN_NAME_SCAN_B_EXIT         "sid_ucmd_scan_b_exit"
#define SID_UCMD_MOD_FN_NAME_SCAN_ERROR          "sid_ucmd_scan_error"

struct sid_ucmd_common_ctx;
struct sid_ucmd_ctx;

typedef int sid_ucmd_mod_fn_t(sid_res_t *mod_res, struct sid_ucmd_common_ctx *ucmd_common_ctx);
typedef int sid_ucmd_fn_t(sid_res_t *mod_res, struct sid_ucmd_ctx *ucmd_ctx);

/*
 * Macros to register module's management functions.
 */

#define SID_UCMD_MOD_PRIO(val)    SID_MOD_PRIO(val)

/*
 * Aliases are encoded as a single string where each alias is delimited by '\0'.
 * For example, "abc\0def\0ijk" defines three aliases - "abc", "def" and "ijk".
 */

#define SID_UCMD_MOD_ALIASES(val) SID_MOD_ALIASES(val)

#ifdef __GNUC__

	#define _SID_UCMD_MOD_FN_SAFE_CAST(fn)                                                                                     \
		(__builtin_choose_expr(__builtin_types_compatible_p(typeof(fn), sid_ucmd_mod_fn_t),                                \
		                       (sid_mod_cb_fn_t *) fn,                                                                     \
		                       (void) 0))

	#define SID_UCMD_MOD_INIT(fn)  SID_MOD_INIT(_SID_UCMD_MOD_FN_SAFE_CAST(fn))
	#define SID_UCMD_MOD_RESET(fn) SID_MOD_RESET(_SID_UCMD_MOD_FN_SAFE_CAST(fn))
	#define SID_UCMD_MOD_EXIT(fn)  SID_MOD_EXIT(_SID_UCMD_MOD_FN_SAFE_CAST(fn))

#else /* __GNUC__ */

	#define SID_UCMD_MOD_FN(name, fn) sid_ucmd_mod_fn_t *sid_ucmd_mod_##name = fn;

	#define SID_UCMD_MOD_INIT(fn)     SID_UCMD_MOD_FN(init, fn) SID_MOD_INIT((module_cb_fn_t *) fn)
	#define SID_UCMD_MOD_RESET(fn)    SID_UCMD_MOD_FN(reset, fn) SID_MOD_RESET((module_cb_fn_t *) fn)
	#define SID_UCMD_MOD_EXIT(fn)     SID_UCMD_MOD_FN(exit, fn) SID_MOD_EXIT((module_cb_fn_t *) fn)

#endif /* __GNUC__ */

/*
 * Macros to register module's phase functions.
 */
#define SID_UCMD_FN(name, fn) sid_ucmd_fn_t *sid_ucmd_##name = fn;

#ifdef __GNUC__

	#define _SID_UCMD_FN_CHECK_TYPE(fn)                                                                                        \
		(__builtin_choose_expr(__builtin_types_compatible_p(typeof(fn), sid_ucmd_fn_t), fn, (void) 0))

#else /* __GNUC__ */

	#define _SID_UCMD_FN_CHECK_TYPE(fn) fn

#endif /* __GNUC__ */

#define SID_UCMD_SCAN_A_INIT(fn)         SID_UCMD_FN(scan_a_init, _SID_UCMD_FN_CHECK_TYPE(fn))
#define SID_UCMD_SCAN_PRE(fn)            SID_UCMD_FN(scan_pre, _SID_UCMD_FN_CHECK_TYPE(fn))
#define SID_UCMD_SCAN_CURRENT(fn)        SID_UCMD_FN(scan_current, _SID_UCMD_FN_CHECK_TYPE(fn))
#define SID_UCMD_SCAN_NEXT(fn)           SID_UCMD_FN(scan_next, _SID_UCMD_FN_CHECK_TYPE(fn))
#define SID_UCMD_SCAN_POST_CURRENT(fn)   SID_UCMD_FN(scan_post_current, _SID_UCMD_FN_CHECK_TYPE(fn))
#define SID_UCMD_SCAN_POST_NEXT(fn)      SID_UCMD_FN(scan_post_next, _SID_UCMD_FN_CHECK_TYPE(fn))
#define SID_UCMD_SCAN_A_EXIT(fn)         SID_UCMD_FN(scan_a_exit, _SID_UCMD_FN_CHECK_TYPE(fn))
#define SID_UCMD_SCAN_REMOVE_INIT(fn)    SID_UCMD_FN(scan_remove_init, _SID_UCMD_FN_CHECK_TYPE(fn))
#define SID_UCMD_SCAN_REMOVE(fn)         SID_UCMD_FN(scan_remove, _SID_UCMD_FN_CHECK_TYPE(fn))
#define SID_UCMD_SCAN_REMOVE_EXIT(fn)    SID_UCMD_FN(scan_remove_exit, _SID_UCMD_FN_CHECK_TYPE(fn))
#define SID_UCMD_SCAN_B_INIT(fn)         SID_UCMD_FN(scan_b_init, _SID_UCMD_FN_CHECK_TYPE(fn))
#define SID_UCMD_SCAN_ACTION_CURRENT(fn) SID_UCMD_FN(scan_action_current, _SID_UCMD_FN_CHECK_TYPE(fn))
#define SID_UCMD_SCAN_ACTION_NEXT(fn)    SID_UCMD_FN(scan_action_next, _SID_UCMD_FN_CHECK_TYPE(fn))
#define SID_UCMD_SCAN_B_EXIT(fn)         SID_UCMD_FN(scan_b_exit, _SID_UCMD_FN_CHECK_TYPE(fn))
#define SID_UCMD_SCAN_ERROR(fn)          SID_UCMD_FN(scan_error, _SID_UCMD_FN_CHECK_TYPE(fn))

/*
 * Functions to retrieve device properties associated with given command ctx.
 */
udev_action_t  sid_ucmd_ev_get_dev_action(struct sid_ucmd_ctx *ucmd_ctx);
udev_devtype_t sid_ucmd_ev_get_dev_type(struct sid_ucmd_ctx *ucmd_ctx);
int            sid_ucmd_ev_get_dev_major(struct sid_ucmd_ctx *ucmd_ctx);
int            sid_ucmd_ev_get_dev_minor(struct sid_ucmd_ctx *ucmd_ctx);
const char    *sid_ucmd_ev_get_dev_path(struct sid_ucmd_ctx *ucmd_ctx);
const char    *sid_ucmd_ev_get_dev_name(struct sid_ucmd_ctx *ucmd_ctx);
uint64_t       sid_ucmd_ev_get_dev_seqnum(struct sid_ucmd_ctx *ucmd_ctx);
uint64_t       sid_ucmd_ev_get_dev_diskseq(struct sid_ucmd_ctx *ucmd_ctx);
const char    *sid_ucmd_ev_get_dev_synth_uuid(struct sid_ucmd_ctx *ucmd_ctx);

typedef enum {
	SID_KV_NS_UNDEFINED, /* namespace not defined */
	SID_KV_NS_UDEV,      /* per-device ns with records in the scope of current device
	                    records automatically imported from udev and all
	                    changed/new records are exported back to udev */
	SID_KV_NS_DEVICE,    /* per-device ns with records in the scope of current device */
	SID_KV_NS_MODULE,    /* per-module ns with records in the scope of current module */
	SID_KV_NS_DEVMOD,    /* per-device ns with records in the scope of current device and module */
	SID_KV_NS_GLOBAL,    /* global ns with records visible for all modules and when processing any device */
} sid_ucmd_kv_namespace_t;

#define SID_KV_FL_NONE    UINT64_C(0x0000000000000000)
#define SID_KV_FL_ALIGN   UINT64_C(0x0000000000000001) /* make sure value's address is aligned to sizeof(void *) */

#define SID_KV_FL_SYNC    UINT64_C(0x0000000000000002) /* synchronize with main KV store */
#define SID_KV_FL_PERSIST UINT64_C(0x0000000000000004) /* make record persistent */
#define SID_KV_FL_SYNC_P  UINT64_C(0x0000000000000006) /* shortcut for KV_SYNC | KV_PERSISTENT */

#define SID_KV_FL_AR      UINT64_C(0x0000000000000008) /* create an archive of current value */

#define SID_KV_FL_RS      UINT64_C(0x0000000000000010) /* reserve key */

#define SID_KV_FL_FRG_RD  UINT64_C(0x0000000000000020) /* foreign modules can read */
#define SID_KV_FL_SUB_RD  UINT64_C(0x0000000000000040) /* subordinate modules can read */
#define SID_KV_FL_SUP_RD  UINT64_C(0x0000000000000080) /* superior modules can read */
#define SID_KV_FL_RD      UINT64_C(0x00000000000000E0) /* shortcut for KV_FRG_RD | KV_SUB_RD | KV_SUP_RD */

#define SID_KV_FL_FRG_WR  UINT64_C(0x0000000000000100) /* foreign modules can write */
#define SID_KV_FL_SUB_WR  UINT64_C(0x0000000000000200) /* subordinate modules can write */
#define SID_KV_FL_SUP_WR  UINT64_C(0x0000000000000400) /* superior modules can write */
#define SID_KV_FL_WR      UINT64_C(0x0000000000000700) /* shortcut for KV_FRG_WR | KV_SUB_WR | KV_SUP_WR */

typedef uint64_t sid_ucmd_kv_flags_t;

#define SID_UCMD_KV_UNSET            ((void *) -1)
#define SID_UCMD_KEY_DEVICE_NEXT_MOD "SID_NEXT_MOD"

const void *sid_ucmd_kv_set(sid_res_t              *mod_res,
                            struct sid_ucmd_ctx    *ucmd_ctx,
                            sid_ucmd_kv_namespace_t ns,
                            const char             *key,
                            const void             *value,
                            size_t                  value_size,
                            sid_ucmd_kv_flags_t     flags);

const void *sid_ucmd_kv_get(sid_res_t              *mod_res,
                            struct sid_ucmd_ctx    *ucmd_ctx,
                            sid_ucmd_kv_namespace_t ns,
                            const char             *key,
                            size_t                 *value_size,
                            sid_ucmd_kv_flags_t    *flags,
                            unsigned int            archive);

const void *sid_ucmd_kv_get_foreign_mod(sid_res_t              *mod_res,
                                        struct sid_ucmd_ctx    *ucmd_ctx,
                                        const char             *foreign_mod_name,
                                        sid_ucmd_kv_namespace_t ns,
                                        const char             *key,
                                        size_t                 *value_size,
                                        sid_ucmd_kv_flags_t    *flags,
                                        unsigned int            archive);

const void *sid_ucmd_kv_get_foreign_dev(sid_res_t              *mod_res,
                                        struct sid_ucmd_ctx    *ucmd_ctx,
                                        const char             *foreign_dev_id,
                                        sid_ucmd_kv_namespace_t ns,
                                        const char             *key,
                                        size_t                 *value_size,
                                        sid_ucmd_kv_flags_t    *flags,
                                        unsigned int            archive);

const void *sid_ucmd_kv_get_foreign_dev_mod(sid_res_t              *mod_res,
                                            struct sid_ucmd_ctx    *ucmd_ctx,
                                            const char             *foreign_dev_id,
                                            const char             *foreign_mod_name,
                                            sid_ucmd_kv_namespace_t ns,
                                            const char             *key,
                                            size_t                 *value_size,
                                            sid_ucmd_kv_flags_t    *flags,
                                            unsigned int            archive);

// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// TODO: The sid_ucmd_kv_get_disk_part doesn't work right now.
//
// 	 Instead, we should be using sid_ucmd_get_foreign_dev_mod_kv with the
// 	 dev ID of the parent disk and then remove this function.
//
// 	 We will be adding ucmd-module.h API to get the layer/list of devices
// 	 underneath given device and also the layer/list of devices above
// 	 (thinking in the form of an iterator). This still needs some thinking
// 	 of a good way how to represent it as we'll be using this for both
// 	 partitions and other devices which are not partitions, but layered
// 	 devices in general.
// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
const void *sid_ucmd_kv_get_disk_part(sid_res_t           *mod_res,
                                      struct sid_ucmd_ctx *ucmd_ctx,
                                      const char          *key,
                                      size_t              *value_size,
                                      sid_ucmd_kv_flags_t *flags);

int sid_ucmd_kv_reserve(sid_res_t                  *mod_res,
                        struct sid_ucmd_common_ctx *ucmd_common_ctx,
                        sid_ucmd_kv_namespace_t     ns,
                        const char                 *key,
                        sid_ucmd_kv_flags_t         flags);

int sid_ucmd_kv_unreserve(sid_res_t                  *mod_res,
                          struct sid_ucmd_common_ctx *ucmd_common_ctx,
                          sid_ucmd_kv_namespace_t     ns,
                          const char                 *key);

typedef enum {
	/* states in which any layers above are not possible */
	SID_DEV_RDY_UNDEFINED,    /* undefined or invalid */
	SID_DEV_RDY_REMOVED,      /* not ready - removed */
	SID_DEV_RDY_UNPROCESSED,  /* not ready - not yet processed by SID */
	SID_DEV_RDY_UNCONFIGURED, /* not ready - not able to perform IO */

	SID_DEV_RDY_UNINITIALIZED, /* ready     - able to perform IO, but not initialized yet */
	SID_DEV_RDY_PRIVATE,       /* ready     - only for private use of the module/subsystem */
	SID_DEV_RDY_FLAT,          /* ready     - publicly available for use, but layers on top disabled */

	/* states in which layers above are possible */
	SID_DEV_RDY_UNAVAILABLE, /* ready     - temporarily unavailable at the moment, e.g. suspended device */
	SID_DEV_RDY_PUBLIC,      /* ready     - publicly available for use, layers on top enabled */

	/* markers for easier state matching */
	_SID_DEV_RDY         = SID_DEV_RDY_UNINITIALIZED,
	_SID_DEV_RDY_LAYERED = SID_DEV_RDY_UNAVAILABLE,
} sid_ucmd_dev_ready_t;

typedef enum {
	SID_DEV_RES_UNDEFINED,   /* undefined or invalid */
	SID_DEV_RES_UNPROCESSED, /* not yet processed by SID */
	SID_DEV_RES_RESERVED,    /* reserved by a layer above */
	SID_DEV_RES_USED,        /* used by a layer above */
	SID_DEV_RES_FREE,        /* not yet reserved or used by a layer above */
} sid_ucmd_dev_reserved_t;

int                  sid_ucmd_dev_set_ready(sid_res_t *mod_res, struct sid_ucmd_ctx *ucmd_ctx, sid_ucmd_dev_ready_t ready);
sid_ucmd_dev_ready_t sid_ucmd_dev_get_ready(sid_res_t *mod_res, struct sid_ucmd_ctx *ucmd_ctx, unsigned int archive);

int sid_ucmd_dev_set_reserved(sid_res_t *mod_res, struct sid_ucmd_ctx *ucmd_ctx, sid_ucmd_dev_reserved_t reserved);
sid_ucmd_dev_reserved_t sid_ucmd_dev_get_reserved(sid_res_t *mod_res, struct sid_ucmd_ctx *ucmd_ctx, unsigned int archive);

int sid_ucmd_dev_alias_add(sid_res_t *mod_res, struct sid_ucmd_ctx *ucmd_ctx, const char *alias_key, const char *alias);

int sid_ucmd_dev_alias_rename(sid_res_t           *mod_res,
                              struct sid_ucmd_ctx *ucmd_ctx,
                              const char          *alias_key,
                              const char          *old_alias,
                              const char          *new_alias);

int sid_ucmd_dev_alias_del(sid_res_t *mod_res, struct sid_ucmd_ctx *ucmd_ctx, const char *alias_key, const char *alias);

const char **sid_ucmd_dev_alias_get(sid_res_t *mod_res, struct sid_ucmd_ctx *ucmd_ctx, const char *alias_key, size_t *count);

const char **sid_ucmd_dev_alias_get_foreign_dev(sid_res_t           *mod_res,
                                                struct sid_ucmd_ctx *ucmd_ctx,
                                                const char          *foreign_dev_id,
                                                const char          *alias_key,
                                                size_t              *count);

const char **sid_ucmd_dev_alias_get_foreign_mod(sid_res_t           *mod_res,
                                                struct sid_ucmd_ctx *ucmd_ctx,
                                                const char          *foreign_mod_name,
                                                const char          *alias_key,
                                                size_t              *count);

const char **sid_ucmd_dev_alias_get_foreign_dev_mod(sid_res_t           *mod_res,
                                                    struct sid_ucmd_ctx *ucmd_ctx,
                                                    const char          *foreign_dev_id,
                                                    const char          *foreign_mod_name,
                                                    const char          *alias_key,
                                                    size_t              *count);

int sid_ucmd_grp_create(sid_res_t              *mod_res,
                        struct sid_ucmd_ctx    *ucmd_ctx,
                        sid_ucmd_kv_namespace_t group_ns,
                        sid_ucmd_kv_flags_t     group_flags,
                        const char             *group_cat,
                        const char             *group_id);

int sid_ucmd_grp_add_current_dev(sid_res_t              *mod_res,
                                 struct sid_ucmd_ctx    *ucmd_ctx,
                                 sid_ucmd_kv_namespace_t group_ns,
                                 const char             *group_cat,
                                 const char             *group_id);

int sid_ucmd_grp_del_current_dev(sid_res_t              *mod_res,
                                 struct sid_ucmd_ctx    *ucmd_ctx,
                                 sid_ucmd_kv_namespace_t group_ns,
                                 const char             *group_cat,
                                 const char             *group_id);

int sid_ucmd_grp_destroy(sid_res_t              *mod_res,
                         struct sid_ucmd_ctx    *ucmd_ctx,
                         sid_ucmd_kv_namespace_t group_ns,
                         const char             *group_cat,
                         const char             *group_id,
                         int                     force);

#ifdef __cplusplus
}
#endif

#endif
