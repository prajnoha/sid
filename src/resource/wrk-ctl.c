/*
 * SPDX-FileCopyrightText: (C) 2017-2024 Red Hat, Inc.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "resource/wrk-ctl.h"

#include "base/buf.h"
#include "base/comms.h"
#include "internal/mem.h"
#include "internal/util.h"
#include "resource/res.h"

#include <dirent.h>
#include <limits.h>
#include <string.h>
#include <sys/prctl.h>
#include <unistd.h>

#ifdef VALGRIND_SUPPORT
	#include <valgrind/valgrind.h>
#endif

#define WORKER_EXT_NAME                  "ext-worker"

#define DEFAULT_WORKER_IDLE_TIMEOUT_USEC 5000000

typedef enum {
	WORKER_CHANNEL_CMD_NOOP,
	WORKER_CHANNEL_CMD_YIELD,
	WORKER_CHANNEL_CMD_DATA,
	WORKER_CHANNEL_CMD_DATA_EXT,
} worker_channel_cmd_t;

#define WORKER_INT_CHANNEL_MIN_BUF_SIZE sizeof(worker_channel_cmd_t)
#define WORKER_EXT_CHANNEL_MIN_BUF_SIZE 4096

static const char *worker_channel_cmd_str[] = {
	[WORKER_CHANNEL_CMD_NOOP]     = "NOOP",
	[WORKER_CHANNEL_CMD_YIELD]    = "YIELD",
	[WORKER_CHANNEL_CMD_DATA]     = "DATA",
	[WORKER_CHANNEL_CMD_DATA_EXT] = "DATA+EXT",
};

static const char *worker_state_str[] = {[SID_WRK_STATE_NEW]       = "WRK_NEW",
                                         [SID_WRK_STATE_IDLE]      = "WRK_IDLE",
                                         [SID_WRK_STATE_ASSIGNED]  = "WRK_ASSIGNED",
                                         [SID_WRK_STATE_EXITING]   = "WRK_EXITING",
                                         [SID_WRK_STATE_TIMED_OUT] = "WRK_TIMED_OUT",
                                         [SID_WRK_STATE_EXITED]    = "WRK_EXITED"};

const sid_res_type_t sid_res_type_wrk_prx;
const sid_res_type_t sid_res_type_wrk_prx_with_ev_loop;
const sid_res_type_t sid_res_type_wrk;

struct worker_init {
	bool                 prepared;
	char                *id;
	struct sid_wrk_chan *channels;
	char               **ext_argv;
	char               **ext_envp;
	void                *arg;
};

struct worker_control {
	sid_wrk_type_t              worker_type;
	struct sid_wrk_init_cb_spec init_cb_spec;
	unsigned                    channel_spec_count;
	struct sid_wrk_chan_spec   *channel_specs;
	struct worker_init          worker_init;
	struct sid_wrk_timeout_spec timeout_spec;
};

struct sid_wrk_chan {
	sid_res_t                      *owner; /* either worker_proxy or worker instance */
	const struct sid_wrk_chan_spec *spec;
	struct sid_buf                 *rx_buf;
	struct sid_buf                 *tx_buf;
	int                             fd;
};

struct worker_kickstart {
	sid_wrk_type_t              type;
	pid_t                       pid;
	struct sid_wrk_chan_spec   *channel_specs;
	struct sid_wrk_chan        *channels;
	unsigned                    channel_count;
	struct sid_wrk_timeout_spec timeout_spec;
	void                       *arg;
};

struct worker_proxy {
	pid_t                       pid;             /* worker PID */
	sid_wrk_type_t              type;            /* worker type */
	sid_wrk_state_t             state;           /* current worker state */
	sid_res_ev_src_t           *idle_timeout_es; /* event source to catch idle timeout for worker */
	sid_res_ev_src_t           *exec_timeout_es; /* event source to catch execution timeout for worker */
	struct sid_wrk_chan        *channels;        /* NULL-terminated array of worker_proxy --> worker channels */
	unsigned                    channel_count;
	struct sid_wrk_timeout_spec timeout_spec;
	void                       *arg;
};

struct worker {
	struct sid_wrk_chan_spec *channel_specs;
	struct sid_wrk_chan      *channels; /* NULL-terminated array of worker --> worker_proxy channels */
	unsigned                  channel_count;
	unsigned                  parent_exited;
	void                     *arg;
};

static void _change_worker_proxy_state(sid_res_t *worker_proxy_res, sid_wrk_state_t state)
{
	struct worker_proxy *worker_proxy = sid_res_get_data(worker_proxy_res);

	sid_res_log_debug(worker_proxy_res,
	                  "Worker state changed: %s --> %s.",
	                  worker_state_str[worker_proxy->state],
	                  worker_state_str[state]);
	worker_proxy->state = state;
}

static int _create_channel(sid_res_t                      *worker_control_res,
                           const struct sid_wrk_chan_spec *spec,
                           struct sid_wrk_chan            *proxy_chan,
                           struct sid_wrk_chan            *chan)
{
	int comms_fds[2];

	proxy_chan->spec = chan->spec = spec;
	proxy_chan->owner = chan->owner = NULL;   /* will be set later with _setup_channel */
	proxy_chan->rx_buf = chan->rx_buf = NULL; /* will be set later with _setup_channel */
	proxy_chan->tx_buf = chan->tx_buf = NULL; /* will be set later with _setup_channel */

	switch (spec->wire.type) {
		case SID_WRK_WIRE_NONE:
			proxy_chan->fd = chan->fd = -1;
			break;

		case SID_WRK_WIRE_PIPE_TO_WRK:
			if (pipe(comms_fds) < 0) {
				proxy_chan->fd = chan->fd = -1;
				sid_res_log_sys_error(worker_control_res, "pipe", "Failed to create pipe to worker.");
				return -1;
			}

			proxy_chan->fd = comms_fds[1];
			chan->fd       = comms_fds[0];
			break;

		case SID_WRK_WIRE_PIPE_TO_PRX:
			if (pipe(comms_fds) < 0) {
				proxy_chan->fd = chan->fd = -1;
				sid_res_log_sys_error(worker_control_res, "pipe", "Failed to create pipe to worker proxy.");
				return -1;
			}

			proxy_chan->fd = comms_fds[0];
			chan->fd       = comms_fds[1];
			break;

		case SID_WRK_WIRE_SOCKET:
			// FIXME: See if SOCK_SEQPACKET would be more appropriate here but looks buffers would
			//        also need to be enhanced to use sendmsg/recvmsg instead of pure read/write somehow.
			if (socketpair(AF_UNIX, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0, comms_fds) < 0) {
				proxy_chan->fd = chan->fd = -1;
				sid_res_log_sys_error(worker_control_res, "socketpair", "Failed to create socket.");
				return -1;
			}

			proxy_chan->fd = comms_fds[0];
			chan->fd       = comms_fds[1];
			break;
	}

	return 0;
}

static int _create_channels(sid_res_t            *worker_control_res,
                            struct sid_wrk_chan **worker_proxy_channels,
                            struct sid_wrk_chan **worker_channels)
{
	struct worker_control *worker_control = sid_res_get_data(worker_control_res);
	struct sid_wrk_chan   *proxy_chans = NULL, *chans = NULL;
	unsigned               i = 0;

	if (!(proxy_chans = mem_zalloc((worker_control->channel_spec_count) * sizeof(struct sid_wrk_chan)))) {
		sid_res_log_error(worker_control_res, "Failed to allocate worker proxy channel array.");
		goto fail;
	}

	if (!(chans = mem_zalloc((worker_control->channel_spec_count) * sizeof(struct sid_wrk_chan)))) {
		sid_res_log_error(worker_control_res, "Failed to allocate worker channel array.");
		goto fail;
	}

	while (i < worker_control->channel_spec_count) {
		if (_create_channel(worker_control_res, &worker_control->channel_specs[i], &proxy_chans[i], &chans[i]) < 0)
			goto fail;
		i++;
	}

	*worker_proxy_channels = proxy_chans;
	*worker_channels       = chans;

	return 0;
fail:
	while (i > 0) {
		i--;
		if (proxy_chans[i].fd >= 0)
			(void) close(proxy_chans[i].fd);
		if (chans[i].fd >= 0)
			(void) close(chans[i].fd);
	}

	if (proxy_chans)
		free(proxy_chans);
	if (chans)
		free(chans);

	return -1;
}

static int _chan_add_rx_data_suffix(const struct sid_wrk_chan *chan)
{
	const struct iovec *suffix =
		sid_wrk_ctl_detect_worker(chan->owner) ? &chan->spec->worker_rx.data_suffix : &chan->spec->proxy_rx.data_suffix;

	if (suffix->iov_base)
		return sid_buf_add(chan->rx_buf, suffix->iov_base, suffix->iov_len, NULL, NULL);

	return 0;
}

#define CHAN_BUF_RECV_MSG 0x1
#define CHAN_BUF_RECV_EOF 0x2

/*
 * Returns:
 *   < 0 error
 *     0 expecting more data
 *     1 MSG
 *     2 EOF
 *     3 MSG + EOF
 */
static int _chan_buf_recv(const struct sid_wrk_chan *chan,
                          uint32_t                   revents,
                          worker_channel_cmd_t      *chan_cmd,
                          struct sid_wrk_data_spec  *data_spec)
{
	// TODO: Double check we're really interested in EPOLLRDHUP.
	bool          hup = (revents & (EPOLLHUP | EPOLLRDHUP)) && !(revents & EPOLLIN);
	ssize_t       n;
	void         *buf_data;
	size_t        buf_data_size;
	unsigned char byte;

	if (revents & EPOLLERR) {
		if (hup)
			sid_res_log_error(chan->owner, "Peer closed channel %s prematurely.", chan->spec->id);
		else
			sid_res_log_error(chan->owner, "Error detected on channel %s.", chan->spec->id);

		return -EPIPE;
	}

	n = sid_buf_read(chan->rx_buf, chan->fd);

	if (n > 0) {
		/* For plain buffers, we are waiting for EOF to complete the message. */
		if (sid_buf_stat(chan->rx_buf).spec.mode == SID_BUF_MODE_PLAIN)
			return 0;

		if (!sid_buf_is_complete(chan->rx_buf, NULL))
			return 0;

		if (_chan_add_rx_data_suffix(chan) < 0)
			return -ENOMEM;

		(void) sid_buf_get_data(chan->rx_buf, (const void **) &buf_data, &buf_data_size);

		/*
		 * Internal workers and associated proxies use SID_BUF_MODE_SIZE_PREFIX buffers and
		 * they always transmit worker_channel_cmd_t as header before actual data.
		 */
		memcpy(chan_cmd, buf_data, sizeof(*chan_cmd));
		data_spec->data_size = buf_data_size - sizeof(*chan_cmd);
		data_spec->data      = data_spec->data_size > 0 ? buf_data + sizeof(*chan_cmd) : NULL;

		if (*chan_cmd == WORKER_CHANNEL_CMD_DATA_EXT) {
			if (chan->spec->wire.type == SID_WRK_WIRE_SOCKET) {
				/* Also read ancillary data in a channel with socket wire - an FD might be passed through this way.
				 */
				/*
				 * FIXME: Buffer is using 'read', but we need to use 'recvmsg' for ancillary data.
				 *        This is why we need to receive the ancillary data separately from usual data here.
				 *        Maybe extend the buffer so it can use 'recvmsg' somehow - a custom callback
				 *        for reading the data? Then we could receive data and anc. data at once in one
				 *        buffer_read call.
				 *
				 * TODO:  Make this a part of event loop instead of looping here!
				 */
				for (;;) {
					n = sid_comms_unix_recv(chan->fd, &byte, sizeof(byte), &data_spec->ext.socket.fd_pass);

					if (n < 0) {
						if (n == -EAGAIN || n == -EINTR)
							continue;

						data_spec->ext.socket.fd_pass = -1;
						sid_res_log_error_errno(chan->owner,
						                        n,
						                        "Failed to read ancillary data on channel %s",
						                        chan->spec->id);

						return n;
					}

					data_spec->ext.used = true;
					break;
				}
			}
		}

		return CHAN_BUF_RECV_MSG;
	} else if (n < 0) {
		if (n == -EAGAIN || n == -EINTR)
			return 0;

		sid_res_log_error_errno(chan->owner, n, "Failed to read data on channel %s", chan->spec->id);
		return n;
	} else {
		if (sid_buf_stat(chan->rx_buf).spec.mode == SID_BUF_MODE_PLAIN) {
			if (_chan_add_rx_data_suffix(chan) < 0)
				return -ENOMEM;

			(void) sid_buf_get_data(chan->rx_buf, (const void **) &buf_data, &buf_data_size);

			*chan_cmd            = WORKER_CHANNEL_CMD_DATA;
			data_spec->data_size = buf_data_size;
			data_spec->data      = buf_data;

			return CHAN_BUF_RECV_EOF | CHAN_BUF_RECV_MSG;
		}

		return CHAN_BUF_RECV_EOF;
	}
}

static int _make_worker_exit(sid_res_t *worker_proxy_res)
{
	struct worker_proxy *worker_proxy = sid_res_get_data(worker_proxy_res);
	int                  r;

	if (!(r = kill(worker_proxy->pid, SIGTERM)))
		_change_worker_proxy_state(worker_proxy_res, SID_WRK_STATE_EXITING);

	return r;
}

static const char _unexpected_internal_command_msg[]    = "unexpected internal command received.";
static const char _custom_message_handling_failed_msg[] = "Custom message handling failed.";

static int _on_worker_proxy_channel_event(sid_res_ev_src_t *es, int fd, uint32_t revents, void *data)
{
	struct sid_wrk_chan     *chan = data;
	worker_channel_cmd_t     chan_cmd;
	struct sid_wrk_data_spec data_spec = {0};
	/*uint64_t timeout_usec;*/
	int r;

	r = _chan_buf_recv(chan, revents, &chan_cmd, &data_spec);

	if (r == 0)
		return 0;

	if (r < 0) {
		if (chan->rx_buf)
			sid_buf_reset(chan->rx_buf);
		return r;
	}

	if (r & CHAN_BUF_RECV_MSG) {
		switch (chan_cmd) {
			case WORKER_CHANNEL_CMD_YIELD:
				/* FIXME: Make timeout configurable. If timeout is set to zero, exit worker right away - call
				_make_worker_exit.
				 *
				timeout_usec = util_get_now_usec(CLOCK_MONOTONIC) + DEFAULT_WORKER_IDLE_TIMEOUT_USEC;
				sid_res_ev_time_create(chan->owner, &worker_proxy->idle_timeout_es, CLOCK_MONOTONIC,
				                                      timeout_usec, 0, _on_worker_proxy_idle_timeout_event, "idle
				timeout", chan->owner); _change_worker_proxy_state(chan->owner, SID_WRK_STATE_IDLE);
				*/
				_make_worker_exit(chan->owner);
				break;
			case WORKER_CHANNEL_CMD_DATA:
			case WORKER_CHANNEL_CMD_DATA_EXT:
				if (chan->spec->proxy_rx.cb.fn) {
					if (chan->spec->proxy_rx.cb.fn(chan->owner, chan, &data_spec, chan->spec->proxy_rx.cb.arg) <
					    0)
						sid_res_log_warning(chan->owner, "%s", _custom_message_handling_failed_msg);
				}
				break;
			default:
				sid_res_log_error(chan->owner,
				                  SID_INTERNAL_ERROR "%s: %s %s",
				                  __func__,
				                  worker_channel_cmd_str[chan_cmd],
				                  _unexpected_internal_command_msg);
		}

		sid_buf_reset(chan->rx_buf);
	}

	if (r & CHAN_BUF_RECV_EOF)
		sid_res_ev_destroy(&es);

	return 0;
}

static int _on_worker_channel_event(sid_res_ev_src_t *es, int fd, uint32_t revents, void *data)
{
	struct sid_wrk_chan     *chan = data;
	worker_channel_cmd_t     chan_cmd;
	struct sid_wrk_data_spec data_spec = {0};
	int                      r;

	r = _chan_buf_recv(chan, revents, &chan_cmd, &data_spec);

	if (r == 0)
		return 0;

	if (r < 0) {
		if (chan->rx_buf)
			sid_buf_reset(chan->rx_buf);
		return r;
	}

	if (r & CHAN_BUF_RECV_MSG) {
		switch (chan_cmd) {
			case WORKER_CHANNEL_CMD_DATA:
			case WORKER_CHANNEL_CMD_DATA_EXT:
				if (chan->spec->worker_rx.cb.fn) {
					if (chan->spec->worker_rx.cb.fn(chan->owner,
					                                chan,
					                                &data_spec,
					                                chan->spec->worker_rx.cb.arg) < 0)
						sid_res_log_warning(chan->owner, "%s", _custom_message_handling_failed_msg);
				}
				break;
			default:
				sid_res_log_error(chan->owner,
				                  SID_INTERNAL_ERROR "%s: %s %s",
				                  __func__,
				                  worker_channel_cmd_str[chan_cmd],
				                  _unexpected_internal_command_msg);
		}

		sid_buf_reset(chan->rx_buf);
	}

	if (r & CHAN_BUF_RECV_EOF)
		sid_res_ev_destroy(&es);

	return 0;
}

static struct sid_buf_init _default_int_wrk_lane_buf_init = {.size = WORKER_INT_CHANNEL_MIN_BUF_SIZE, .alloc_step = 1, .limit = 0};

static struct sid_buf_init _default_ext_wrk_lane_buf_init = {.size       = WORKER_EXT_CHANNEL_MIN_BUF_SIZE,
                                                             .alloc_step = WORKER_EXT_CHANNEL_MIN_BUF_SIZE,
                                                             .limit      = 0};

static const struct sid_buf_init *_get_lane_buf_init(const struct sid_buf_init *buf_init,
                                                     const struct sid_buf_init *default_buf_init)
{
	if (buf_init && (buf_init->size || buf_init->alloc_step))
		return buf_init;

	return default_buf_init;
}

static int _setup_channel(sid_res_t *owner, bool is_worker, sid_wrk_type_t type, struct sid_wrk_chan *chan)
{
	struct sid_buf_spec        buf_spec = {0};
	struct sid_buf           **buf1, **buf2;
	const struct sid_buf_init *buf1_init, *buf2_init;
	int                        r;

	if (chan->rx_buf || chan->tx_buf) {
		sid_res_log_error(owner, SID_INTERNAL_ERROR "%s: Buffers already set.", __func__);
		r = -EINVAL;
		goto fail;
	}

	/*
	 * Buffer wiring scheme, assigned buffer to buf1 and buf2 is enclosed with '[...]':
	 *
	 * WORKER:
	 *   SID_WRK_TYPE_INTERNAL (A):
	 *     buf1: [ worker/tx_buf ] -> proxy/rx_buf
	 *     buf2: [ worker/rx_buf ] <- proxy/tx_buf
	 *
	 *   SID_WRK_TYPE_EXTERNAL (B):
	 *     external worker - we have no control over buffers here
	 *
	 *
	 * PROXY:
	 *   SID_WRK_TYPE_INTERNAL (C):
	 *   SID_WRK_TYPE_EXTERNAL (D):
	 *     buf1:   worker/tx_buf -> [ proxy/rx_buf ]
	 *     buf2:   worker/rx_buf <- [ proxy/tx_buf ]
	 *
	 * For internal workers, we have complete control on how data are sent and so we mandate
	 * use of data size prefixes on both sides of the channel so we always know how much data
	 * to receive on the other side and we can preallocate proper buffer size for it.
	 *
	 * We can't use data size prefix for external workers - they simply send us plain data.
	 * Since we don't know in advance how much data to accept, we use WORKER_EXT_CHANNEL_MIN_BUF_SIZE
	 * and then we extend the buffer with WORKER_EXT_CHANNEL_MIN_BUF_SIZE each time it's filled up
	 * and data are still incoming.
	 */
	if (is_worker) {
		if (type == SID_WRK_TYPE_INTERNAL) {
			/* (A) */
			buf1          = &chan->tx_buf;
			buf2          = &chan->rx_buf;

			buf_spec.mode = SID_BUF_MODE_SIZE_PREFIX;
			buf1_init     = _get_lane_buf_init(&chan->spec->worker_tx.buf_init, &_default_int_wrk_lane_buf_init);
			buf2_init     = _get_lane_buf_init(&chan->spec->worker_rx.buf_init, &_default_int_wrk_lane_buf_init);
		} else {
			/* (B) */
			buf1 = NULL;
			buf2 = NULL;
		}
	} else {
		/* (C), (D) */
		buf1 = &chan->rx_buf;
		buf2 = &chan->tx_buf;

		if (type == SID_WRK_TYPE_INTERNAL) {
			/* (C) */
			buf_spec.mode = SID_BUF_MODE_SIZE_PREFIX;
			buf1_init     = _get_lane_buf_init(&chan->spec->proxy_rx.buf_init, &_default_int_wrk_lane_buf_init);
			buf2_init     = _get_lane_buf_init(&chan->spec->proxy_tx.buf_init, &_default_int_wrk_lane_buf_init);
		} else {
			/* (D) */
			buf_spec.mode = SID_BUF_MODE_PLAIN;
			buf1_init     = _get_lane_buf_init(&chan->spec->proxy_rx.buf_init, &_default_ext_wrk_lane_buf_init);
			buf2_init     = _get_lane_buf_init(&chan->spec->proxy_tx.buf_init, &_default_ext_wrk_lane_buf_init);
		}
	}

	switch (chan->spec->wire.type) {
		case SID_WRK_WIRE_NONE:
			break;

		case SID_WRK_WIRE_PIPE_TO_WRK:
			if (buf2) {
				if (!(*buf2 = sid_buf_create(&buf_spec, buf2_init, &r))) {
					sid_res_log_error_errno(owner,
					                        r,
					                        "Failed to create buffer for channel with ID %s.",
					                        chan->spec->id);
					goto fail;
				}
			}

			if (is_worker && chan->spec->wire.ext.used && chan->spec->wire.ext.pipe.fd_redir >= 0) {
				if (dup2(chan->fd, chan->spec->wire.ext.pipe.fd_redir) < 0) {
					sid_res_log_error_errno(owner,
					                        errno,
					                        "Failed to redirect FD %d through channel %s",
					                        chan->spec->wire.ext.pipe.fd_redir,
					                        chan->spec->id);
					r = -errno;
					goto fail;
				}
				(void) close(chan->fd);
				chan->fd = chan->spec->wire.ext.pipe.fd_redir;
			}

			break;

		case SID_WRK_WIRE_PIPE_TO_PRX:
			if (buf1) {
				if (!(*buf1 = sid_buf_create(&buf_spec, buf1_init, &r)))
					goto fail;
			}

			if (is_worker && chan->spec->wire.ext.used && chan->spec->wire.ext.pipe.fd_redir >= 0) {
				if (dup2(chan->fd, chan->spec->wire.ext.pipe.fd_redir) < 0) {
					sid_res_log_error_errno(owner,
					                        errno,
					                        "Failed to redirect FD %d through channel %s",
					                        chan->spec->wire.ext.pipe.fd_redir,
					                        chan->spec->id);
					r = -errno;
					goto fail;
				}
				(void) close(chan->fd);
				chan->fd = chan->spec->wire.ext.pipe.fd_redir;
			}

			break;

		case SID_WRK_WIRE_SOCKET:
			if ((buf1 && !(*buf1 = sid_buf_create(&buf_spec, buf1_init, &r))) ||
			    (buf2 && !(*buf2 = sid_buf_create(&buf_spec, buf2_init, &r)))) {
				sid_res_log_error_errno(owner,
				                        r,
				                        "Failed to create buffer for channel with ID %s.",
				                        chan->spec->id);
				goto fail;
			}

			if (is_worker && chan->spec->wire.ext.used && chan->spec->wire.ext.socket.fd_redir >= 0) {
				if (dup2(chan->fd, chan->spec->wire.ext.socket.fd_redir) < 0) {
					sid_res_log_error_errno(owner,
					                        errno,
					                        "Failed to redirect FD %d through channel %s : WORKER_WIRE_SOCKET",
					                        chan->spec->wire.ext.pipe.fd_redir,
					                        chan->spec->id);
				}

				(void) close(chan->fd);
				chan->fd = chan->spec->wire.ext.socket.fd_redir;
			}
			break;
	}

	if (!(type == SID_WRK_TYPE_EXTERNAL && is_worker)) {
		if (sid_res_ev_create_io(owner,
		                         NULL,
		                         chan->fd,
		                         is_worker ? _on_worker_channel_event : _on_worker_proxy_channel_event,
		                         0,
		                         chan->spec->id,
		                         chan) < 0) {
			sid_res_log_error(owner, "Failed to register communication channel with ID %s.", chan->spec->id);
			r = -1;
			goto fail;
		}

		chan->owner = owner;
	}

	return 0;
fail:
	if (chan->rx_buf) {
		sid_buf_destroy(chan->rx_buf);
		chan->rx_buf = NULL;
	}

	if (chan->tx_buf) {
		sid_buf_destroy(chan->tx_buf);
		chan->tx_buf = NULL;
	}

	return r;
}

static int _setup_channels(sid_res_t *owner, sid_wrk_type_t type, struct sid_wrk_chan *chans, unsigned chan_count)
{
	bool     is_worker;
	unsigned i = 0;

	if (sid_res_match(owner, &sid_res_type_wrk_ctl, NULL))
		is_worker = true;
	else
		is_worker = sid_wrk_ctl_detect_worker(owner);

	while (i < chan_count) {
		if (_setup_channel(owner, is_worker, type, &chans[i]) < 0)
			goto fail;
		i++;
	}

	return 0;
fail:
	/*
	 * If _setup_channel failed, it already cleaned up channel buffers for the exact channel
	 * it was processing. Here, we clean up all the buffers for channels already setup right
	 * before this failure.
	 */
	while (i > 0) {
		i--;

		if (chans[i].rx_buf) {
			sid_buf_destroy(chans[i].rx_buf);
			chans[i].rx_buf = NULL;
		}
		if (chans[i].tx_buf) {
			sid_buf_destroy(chans[i].tx_buf);
			chans[i].tx_buf = NULL;
		}
	}

	return -1;
}

static void _destroy_channels(struct sid_wrk_chan *channels, unsigned channel_count)
{
	unsigned             i;
	struct sid_wrk_chan *chan;

	for (i = 0; i < channel_count; i++) {
		chan = &channels[i];

		switch (chan->spec->wire.type) {
			case SID_WRK_WIRE_NONE:
				break;
			case SID_WRK_WIRE_SOCKET:
			case SID_WRK_WIRE_PIPE_TO_WRK:
			case SID_WRK_WIRE_PIPE_TO_PRX:
				if (chan->fd >= 0)
					(void) close(chan->fd);
				break;
		}

		if (chan->rx_buf)
			sid_buf_destroy(chan->rx_buf);

		if (chan->tx_buf)
			sid_buf_destroy(chan->tx_buf);
	}

	free(channels);
}

static int _close_non_channel_fds(sid_res_t *res, struct sid_wrk_chan *channels, unsigned channel_count)
{
	static const char *proc_self_fd_dir = SYSTEM_PROC_PATH "/self/fd";
	DIR               *d;
	struct dirent     *dirent;
	char              *p;
	int                fd, i, r = -1;

#ifdef VALGRIND_SUPPORT
	if (RUNNING_ON_VALGRIND)
		return 0;
#endif

	if (!(d = opendir(proc_self_fd_dir))) {
		sid_res_log_sys_error(res, "opendir", proc_self_fd_dir);
		goto out;
	}

	while ((dirent = readdir(d))) {
		errno = 0;
		fd    = strtol(dirent->d_name, &p, 10);
		if (errno || !p || *p || fd >= INT_MAX)
			continue;

		if (fd == dirfd(d))
			continue;

		for (i = 0; i < channel_count; i++) {
			if (channels[i].fd == fd)
				break;
		}

		if (i < channel_count)
			continue;

		if (close(fd) < 0) {
			sid_res_log_sys_error(res, "close non-channel fd", dirent->d_name);
			goto out;
		}

		sid_res_log_debug(res, "Closed non-channel fd %d.", fd);
	}

	r = 0;
out:
	if (d) {
		if (closedir(d) < 0) {
			sid_res_log_sys_error(res, "closedir", proc_self_fd_dir);
			r = -1;
		}
	}
	return r;
}

static int _do_worker_control_get_new_worker(sid_res_t             *worker_control_res,
                                             struct sid_wrk_params *params,
                                             sid_res_t            **res_p,
                                             bool                   with_event_loop)
{
	struct worker_control  *worker_control        = sid_res_get_data(worker_control_res);
	struct sid_wrk_chan    *worker_proxy_channels = NULL, *worker_channels = NULL;
	struct worker_kickstart kickstart;
	sigset_t                original_sigmask, new_sigmask;
	pid_t                   original_pid, curr_ppid;
	sid_res_t              *res             = NULL;
	int                     signals_blocked = 0;
	pid_t                   pid             = -1;
	const char             *id;
	char                    gen_id[32];

	*res_p = NULL;

	if (_create_channels(worker_control_res, &worker_proxy_channels, &worker_channels) < 0) {
		sid_res_log_error(worker_control_res, "Failed to create worker channels.");
		goto out;
	}

	if (sigfillset(&new_sigmask) < 0) {
		sid_res_log_sys_error(worker_control_res, "sigfillset", "");
		goto out;
	}

	if (sigprocmask(SIG_SETMASK, &new_sigmask, &original_sigmask) < 0) {
		sid_res_log_sys_error(worker_control_res, "sigprocmask", "blocking signals before fork");
		goto out;
	}
	signals_blocked = 1;
	original_pid    = getpid();

	if ((pid = fork()) < 0) {
		sid_res_log_sys_error(worker_control_res, "fork", "");
		goto out;
	}

	if (pid == 0) {
		/*
		 *  WORKER HERE
		 */

		if (worker_control->worker_type == SID_WRK_TYPE_INTERNAL) {
			/* SID_WRK_TYPE_INTERNAL - request a SIGUSR1 signal if parent dies */
			if (prctl(PR_SET_PDEATHSIG, SIGUSR1) < 0)
				sid_res_log_sys_error(worker_control_res,
				                      "prctl",
				                      "failed to set parent-death signal for internal worker");
		} else {
			/* SID_WRK_TYPE_EXTERNAL - request a SIGTERM signal if parent dies */
			if (prctl(PR_SET_PDEATHSIG, SIGTERM) < 0)
				sid_res_log_sys_error(worker_control_res,
				                      "prctl",
				                      "failed to set parent-death signal for external worker");
		}

		/* Check to make sure the parent didn't die right after the fork() */
		curr_ppid = getppid();
		if (curr_ppid != original_pid) {
			sid_res_log_debug(worker_control_res,
			                  "parent died before prctl() call completed - exiting SID worker process");
			raise(SIGTERM);
			exit(EXIT_FAILURE);
		}

		_destroy_channels(worker_proxy_channels, worker_control->channel_spec_count);

		worker_proxy_channels                = NULL;
		worker_control->worker_init.channels = worker_channels;

		if (worker_control->worker_type == SID_WRK_TYPE_INTERNAL)
			worker_control->worker_init.id = params->id ? strdup(params->id) : NULL;
		else {
			/*
			 * SID_WRK_TYPE_EXTERNAL
			 */
			if (_close_non_channel_fds(worker_control_res, worker_channels, worker_control->channel_spec_count) < 0)
				exit(EXIT_FAILURE);

			if (!params->id || asprintf(&worker_control->worker_init.id, "%s/%s", WORKER_EXT_NAME, params->id) < 0)
				worker_control->worker_init.id = NULL;

			if (!(worker_control->worker_init.ext_argv = util_str_comb_to_strv(NULL,
			                                                                   params->external.exec_file,
			                                                                   params->external.args,
			                                                                   NULL,
			                                                                   UTIL_STR_DEFAULT_DELIMS,
			                                                                   UTIL_STR_DEFAULT_QUOTES)) ||
			    !(worker_control->worker_init.ext_envp = util_str_comb_to_strv(NULL,
			                                                                   NULL,
			                                                                   params->external.env,
			                                                                   NULL,
			                                                                   UTIL_STR_DEFAULT_DELIMS,
			                                                                   UTIL_STR_DEFAULT_QUOTES))) {
				sid_res_log_error(
					worker_control_res,
					"Failed to convert argument and environment strings to vectors for external worker %s.",
					worker_control->worker_init.id ?: WORKER_EXT_NAME);
				exit(EXIT_FAILURE);
			}
		}

		worker_control->worker_init.arg      = params->worker_arg;
		worker_control->worker_init.prepared = true;

		/* worker processes return with *res_p == NULL, to differentiate them from the proxy process*/
		return 0;
	}
	/*
	 * WORKER PROXY HERE
	 */

	sid_res_log_debug(worker_control_res, "Created new worker process with PID %d.", pid);

	_destroy_channels(worker_channels, worker_control->channel_spec_count);
	worker_channels         = NULL;

	kickstart.type          = worker_control->worker_type;
	kickstart.pid           = pid;
	kickstart.channels      = worker_proxy_channels;
	kickstart.channel_count = worker_control->channel_spec_count;
	kickstart.arg           = params->worker_proxy_arg;

	if (params->timeout_spec.usec)
		/* override default timeout for this single worker */
		kickstart.timeout_spec = params->timeout_spec;
	else
		/* use default timeout */
		kickstart.timeout_spec = worker_control->timeout_spec;

	if (!(id = params->id)) {
		(void) util_proc_pid_to_str(kickstart.pid, gen_id, sizeof(gen_id));
		id = gen_id;
	}

	res = sid_res_create(worker_control_res,
	                     with_event_loop ? &sid_res_type_wrk_prx_with_ev_loop : &sid_res_type_wrk_prx,
	                     SID_RES_FL_DISALLOW_ISOLATION,
	                     id,
	                     &kickstart,
	                     SID_RES_PRIO_NORMAL,
	                     SID_RES_NO_SERVICE_LINKS);
out:
	if (!res) {
		if (worker_proxy_channels)
			_destroy_channels(worker_proxy_channels, worker_control->channel_spec_count);

		if (worker_channels)
			_destroy_channels(worker_channels, worker_control->channel_spec_count);

		// FIXME: also terminate worker if worker_proxy creation fails
	}

	if (signals_blocked && sigprocmask(SIG_SETMASK, &original_sigmask, NULL) < 0)
		sid_res_log_sys_error(worker_control_res, "sigprocmask", "after forking process");

	/* return worker proxy resource */
	*res_p = res;
	return res ? 0 : -1;
}

int sid_wrk_ctl_get_new_worker(sid_res_t *worker_control_res, struct sid_wrk_params *params, sid_res_t **res_p)
{
	if (!sid_res_match(worker_control_res, &sid_res_type_wrk_ctl, NULL) || !params || !res_p)
		return -EINVAL;

	return _do_worker_control_get_new_worker(worker_control_res, params, res_p, false);
}

static int _run_internal_worker(sid_res_t *worker_control_res, sid_res_srv_lnk_def_t service_link_defs[])
{
	struct worker_control  *worker_control = sid_res_get_data(worker_control_res);
	struct worker_kickstart kickstart;
	sid_res_t              *res;
	const char             *id;
	char                    gen_id[32];

	kickstart.type          = SID_WRK_TYPE_INTERNAL;
	kickstart.pid           = getpid();
	kickstart.channels      = worker_control->worker_init.channels;
	kickstart.channel_count = worker_control->channel_spec_count;
	kickstart.channel_specs = worker_control->channel_specs;
	kickstart.arg           = worker_control->worker_init.arg;

	if (!(id = worker_control->worker_init.id)) {
		(void) util_proc_pid_to_str(kickstart.pid, gen_id, sizeof(gen_id));
		id = gen_id;
	}

	res = sid_res_create(SID_RES_NO_PARENT,
	                     &sid_res_type_wrk,
	                     SID_RES_FL_NONE,
	                     id,
	                     &kickstart,
	                     SID_RES_PRIO_NORMAL,
	                     service_link_defs);
	if (!res) {
		(void) sid_res_unref(sid_res_search(worker_control_res, SID_RES_SEARCH_TOP, NULL, NULL));
		return -1;
	}
	/*
	 * We are going to destroy the worker_control and the worker already
	 * has a reference to the channels and the channel_specs. Set them
	 * to NULL, so they aren't freed when the worker_control is destroyed
	 */
	worker_control->worker_init.channels = NULL;
	worker_control->channel_specs        = NULL;

	if (worker_control->init_cb_spec.fn)
		(void) worker_control->init_cb_spec.fn(res, worker_control->init_cb_spec.arg);

	return sid_res_ev_loop_run(res);
}

static int _run_external_worker(sid_res_t *worker_control_res)
{
	struct worker_control    *worker_control = sid_res_get_data(worker_control_res);
	struct sid_wrk_chan_spec *channel_specs;
	struct sid_wrk_chan      *channels;
	unsigned                  channel_count;
	char                    **argv, **envp;
	char                     *id;
	char                      gen_id[32];
	int                       r;

	/*
	 * We may destroy the worker_control. Save references to the values we
	 * need to keep, and set them to NULL, so they aren't freed if the
	 * worker_control is destroyed
	 */
	if (!(id = worker_control->worker_init.id)) {
		snprintf(gen_id, sizeof(gen_id), "%s/%d", WORKER_EXT_NAME, getpid());
		id = gen_id;
	}
	worker_control->worker_init.id       = NULL;

	channels                             = worker_control->worker_init.channels;
	worker_control->worker_init.channels = NULL;
	channel_count                        = worker_control->channel_spec_count;

	channel_specs                        = worker_control->channel_specs;
	worker_control->channel_specs        = NULL;

	argv                                 = worker_control->worker_init.ext_argv;
	worker_control->worker_init.ext_argv = NULL;

	envp                                 = worker_control->worker_init.ext_envp;
	worker_control->worker_init.ext_envp = NULL;

	if ((r = _setup_channels(worker_control_res, SID_WRK_TYPE_EXTERNAL, channels, channel_count)) < 0)
		goto fail;

	if (worker_control->init_cb_spec.fn && (r = worker_control->init_cb_spec.fn(NULL, worker_control->init_cb_spec.arg)) < 0)
		goto fail;

	r = execve(argv[0], argv, envp);
	/* On success, execve never returns */
	sid_res_log_error_errno(worker_control_res, errno, "Failed to execute external command %s (%s)", argv[0], id);
fail:
	if (id != gen_id)
		free(id);
	_destroy_channels(channels, channel_count);
	free(channel_specs);
	free(argv);
	free(envp);
	return r;
}

int sid_wrk_ctl_run_worker(sid_res_t *worker_control_res, sid_res_srv_lnk_def_t service_link_defs[])
{
	struct worker_control *worker_control;

	if (!sid_res_match(worker_control_res, &sid_res_type_wrk_ctl, NULL))
		return -EINVAL;

	worker_control = sid_res_get_data(worker_control_res);

	if (!worker_control->worker_init.prepared)
		return -ESRCH;

	worker_control->worker_init.prepared = false;

	if (worker_control->worker_type == SID_WRK_TYPE_INTERNAL)
		return _run_internal_worker(worker_control_res, service_link_defs);

	return _run_external_worker(worker_control_res);
}

/*
 * FIXME: Cleanup resources before running the external worker or do
 *        something to make valgrind happy, otherwise it will report memleaks.
 */
int sid_wrk_ctl_run_new_worker(sid_res_t             *worker_control_res,
                               struct sid_wrk_params *params,
                               sid_res_srv_lnk_def_t  service_link_defs[])
{
	struct worker_control *worker_control;
	sid_res_t             *proxy_res;
	int                    r;

	if (!sid_res_match(worker_control_res, &sid_res_type_wrk_ctl, NULL) || !params)
		return -EINVAL;

	worker_control = sid_res_get_data(worker_control_res);

	if (worker_control->worker_type != SID_WRK_TYPE_EXTERNAL)
		return -ENOTSUP;

	if (worker_control->worker_init.prepared)
		return -EBUSY;

	if ((r = _do_worker_control_get_new_worker(worker_control_res, params, &proxy_res, true)) < 0)
		return r;

	if (proxy_res)
		/*
		 * WORKER PROXY HERE
		 */
		return sid_res_ev_loop_run(proxy_res);
	else
		/*
		 * WORKER HERE
		 */
		return sid_wrk_ctl_run_worker(worker_control_res, service_link_defs);
}

sid_res_t *sid_wrk_ctl_get_idle_worker(sid_res_t *worker_control_res)
{
	sid_res_iter_t *iter;
	sid_res_t      *res;

	if (!sid_res_match(worker_control_res, &sid_res_type_wrk_ctl, NULL))
		return NULL;

	if (!(iter = sid_res_iter_create(worker_control_res)))
		return NULL;

	while ((res = sid_res_iter_next(iter))) {
		if (((struct worker_proxy *) sid_res_get_data(res))->state == SID_WRK_STATE_IDLE)
			break;
	}

	sid_res_iter_destroy(iter);
	return res;
}

sid_res_t *sid_wrk_ctl_find_worker(sid_res_t *worker_control_res, const char *id)
{
	if (!sid_res_match(worker_control_res, &sid_res_type_wrk_ctl, NULL) || UTIL_STR_EMPTY(id))
		return NULL;

	return sid_res_search(worker_control_res, SID_RES_SEARCH_IMM_DESC, &sid_res_type_wrk_prx, id);
}

bool sid_wrk_ctl_detect_worker(sid_res_t *res)
{
	if (!res)
		return NULL;

	// TODO: detect external worker
	if (sid_res_match(res, &sid_res_type_wrk, NULL))
		return true;
	else if (sid_res_match(res, &sid_res_type_wrk_prx, NULL))
		return false;
	else
		return sid_res_search(res, SID_RES_SEARCH_ANC, &sid_res_type_wrk, NULL) != NULL;
}

sid_wrk_state_t sid_wrk_ctl_get_worker_state(sid_res_t *res)
{
	struct worker_proxy *worker_proxy;

	if (!res)
		return SID_WRK_STATE_UNKNOWN;

	do {
		if (sid_res_match(res, &sid_res_type_wrk_prx, NULL) ||
		    sid_res_match(res, &sid_res_type_wrk_prx_with_ev_loop, NULL)) {
			worker_proxy = sid_res_get_data(res);
			return worker_proxy->state;
		}
	} while ((res = sid_res_search(res, SID_RES_SEARCH_IMM_ANC, NULL, NULL)));

	return SID_WRK_STATE_UNKNOWN;
}

const char *sid_wrk_ctl_get_worker_id(sid_res_t *res)
{
	if (!res)
		return NULL;

	do {
		if (sid_res_match(res, &sid_res_type_wrk, NULL) || sid_res_match(res, &sid_res_type_wrk_prx, NULL) ||
		    sid_res_match(res, &sid_res_type_wrk_prx_with_ev_loop, NULL))
			return sid_res_get_id(res);
	} while ((res = sid_res_search(res, SID_RES_SEARCH_IMM_ANC, NULL, NULL)));

	return NULL;
}

void *sid_wrk_ctl_get_worker_arg(sid_res_t *res)
{
	if (!res)
		return NULL;

	do {
		if (sid_res_match(res, &sid_res_type_wrk, NULL))
			return (((struct worker *) sid_res_get_data(res))->arg);
		else if (sid_res_match(res, &sid_res_type_wrk_prx, NULL) ||
		         sid_res_match(res, &sid_res_type_wrk_prx_with_ev_loop, NULL))
			return (((struct worker_proxy *) sid_res_get_data(res))->arg);
	} while ((res = sid_res_search(res, SID_RES_SEARCH_IMM_ANC, NULL, NULL)));

	return NULL;
}

/* FIXME: Consider making this a part of event loop. */
static int _chan_buf_send(const struct sid_wrk_chan *chan, worker_channel_cmd_t chan_cmd, struct sid_wrk_data_spec *data_spec)
{
	static unsigned char byte     = 0xFF;
	int                  has_data = data_spec && data_spec->data && data_spec->data_size;
	ssize_t              n;
	int                  r = 0;

	/*
	 * Internal workers and associated proxies use SID_BUF_MODE_SIZE_PREFIX buffers and
	 * they always transmit worker_channel_cmd_t as header before actual data.
	 * FIXME: avoid using SID_BUF_MODE_SIZE_PREFIX for this detection
	 */
	if (sid_buf_stat(chan->tx_buf).spec.mode == SID_BUF_MODE_SIZE_PREFIX &&
	    (sid_buf_add(chan->tx_buf, &chan_cmd, sizeof(chan_cmd), NULL, NULL)) < 0) {
		r = -ENOMEM;
		goto out;
	}

	if (has_data && (sid_buf_add(chan->tx_buf, data_spec->data, data_spec->data_size, NULL, NULL) < 0)) {
		r = -ENOMEM;
		goto out;
	}

	if ((r = sid_buf_write_all(chan->tx_buf, chan->fd)) < 0) {
		sid_res_log_error_errno(chan->owner, r, "Failed to write data on channel %s", chan->spec->id);
		goto out;
	}

	if (data_spec && data_spec->ext.used) {
		if (chan->spec->wire.type == SID_WRK_WIRE_SOCKET) {
			/* Also send ancillary data in a channel with socket wire - an FD might be passed through this way. */
			/*
			 * FIXME: Buffer is using 'write', but we need to use 'sendmsg' (wrapped by sid_comms_unix_send) for
			 * ancillary data. This is why we need to send the ancillary data separately from usual data here. Maybe
			 * extend the buffer so it can use 'sendmsg' somehow - a custom callback for writing the data? Then we could
			 * send data and anc. data at once in one buffer_write call.
			 */
			for (;;) {
				n = sid_comms_unix_send(chan->fd, &byte, sizeof(byte), data_spec->ext.socket.fd_pass);

				if (n < 0) {
					if (n == -EAGAIN || n == -EINTR)
						continue;

					sid_res_log_error_errno(chan->owner,
					                        n,
					                        "Failed to send ancillary data on channel %s",
					                        chan->spec->id);
					r = n;
					goto out;
				}

				break;
			}
		}
	}
out:
	(void) sid_buf_reset(chan->tx_buf);
	return r;
}

static struct sid_wrk_chan *_get_channel(struct sid_wrk_chan *channels, unsigned channel_count, const char *channel_id)
{
	struct sid_wrk_chan *chan;
	unsigned             i;

	for (i = 0; i < channel_count; i++) {
		chan = &channels[i];
		if (!strcmp(chan->spec->id, channel_id))
			return chan;
	}

	return NULL;
}

static int _channel_prepare_send(sid_res_t                *current_res,
                                 const char               *channel_id,
                                 struct sid_wrk_data_spec *data_spec,
                                 struct sid_wrk_chan     **found_chan)
{
	sid_res_t           *res;
	struct worker_proxy *worker_proxy;
	struct worker       *worker;
	struct sid_wrk_chan *chan;

	if (!current_res || !data_spec)
		return -EINVAL;

	if (UTIL_STR_EMPTY(channel_id))
		return -ECHRNG;

	res = current_res;

	if (sid_res_match(res, &sid_res_type_wrk_prx, NULL) ||
	    (res = sid_res_search(current_res, SID_RES_SEARCH_ANC, &sid_res_type_wrk_prx, NULL))) {
		/* sending from worker proxy to worker */
		worker_proxy = sid_res_get_data(res);

		if (!(chan = _get_channel(worker_proxy->channels, worker_proxy->channel_count, channel_id)))
			return -ECHRNG;

		if (worker_proxy->idle_timeout_es)
			sid_res_ev_destroy(&worker_proxy->idle_timeout_es);
		if (worker_proxy->state != SID_WRK_STATE_ASSIGNED)
			_change_worker_proxy_state(res, SID_WRK_STATE_ASSIGNED);

		if (chan->spec->proxy_tx.cb.fn)
			if (chan->spec->proxy_tx.cb.fn(res, chan, data_spec, chan->spec->proxy_tx.cb.arg) < 0)
				sid_res_log_warning(current_res, "%s", _custom_message_handling_failed_msg);

	} else if ((res = sid_res_search(current_res, SID_RES_SEARCH_TOP, &sid_res_type_wrk, NULL))) {
		/* sending from worker to worker proxy */
		worker = sid_res_get_data(res);

		if (!(chan = _get_channel(worker->channels, worker->channel_count, channel_id)))
			return -ECHRNG;

		if (chan->spec->worker_tx.cb.fn)
			if (chan->spec->worker_tx.cb.fn(res, chan, data_spec, chan->spec->worker_tx.cb.arg) < 0)
				sid_res_log_warning(current_res, "%s", _custom_message_handling_failed_msg);

	} else
		return -ENOMEDIUM;

	*found_chan = chan;
	return 0;
}

int sid_wrk_ctl_chan_send(sid_res_t *current_res, const char *channel_id, struct sid_wrk_data_spec *data_spec)
{
	struct sid_wrk_chan *chan;
	int                  r;

	if ((r = _channel_prepare_send(current_res, channel_id, data_spec, &chan)) < 0)
		return r;

	return _chan_buf_send(chan, data_spec->ext.used ? WORKER_CHANNEL_CMD_DATA_EXT : WORKER_CHANNEL_CMD_DATA, data_spec);
}

int sid_wrk_ctl_chan_close(sid_res_t *current_res, const char *channel_id)
{
	struct sid_wrk_chan *chan;
	int                  r;

	if ((r = _channel_prepare_send(current_res, channel_id, &SID_WRK_DATA_SPEC(), &chan)) < 0)
		return r;

	(void) close(chan->fd);
	chan->fd = -1;

	return 0;
}

int sid_wrk_ctl_yield_worker(sid_res_t *res)
{
	sid_res_t           *worker_res;
	struct worker       *worker;
	struct sid_wrk_chan *chan;
	unsigned             i;

	if (!res)
		return -EINVAL;

	if (sid_res_match(res, &sid_res_type_wrk, NULL))
		worker_res = res;
	else if (!(worker_res = sid_res_search(res, SID_RES_SEARCH_ANC, &sid_res_type_wrk, NULL)))
		return -ENOMEDIUM;

	worker = sid_res_get_data(worker_res);

	for (i = 0; i < worker->channel_count; i++) {
		chan = &worker->channels[i];
		if (chan->spec->wire.type == SID_WRK_WIRE_PIPE_TO_PRX || chan->spec->wire.type == SID_WRK_WIRE_SOCKET) {
			if (worker->parent_exited == 0)
				return _chan_buf_send(chan, WORKER_CHANNEL_CMD_YIELD, NULL);
			else
				raise(SIGTERM);
		}
	}

	return -ENOTCONN;
}

static int _on_worker_proxy_child_event(sid_res_ev_src_t *es, const siginfo_t *si, void *data)
{
	sid_res_t *worker_proxy_res = data;

	switch (si->si_code) {
		case CLD_EXITED:
			sid_res_log_debug(worker_proxy_res, "Worker exited with exit code %d.", si->si_status);
			break;
		case CLD_KILLED:
		case CLD_DUMPED:
			sid_res_log_debug(worker_proxy_res,
			                  "Worker terminated by signal %d (SIG%s).",
			                  si->si_status,
			                  sigabbrev_np(si->si_status));
			break;
		default:
			sid_res_log_debug(worker_proxy_res, "Worker failed unexpectedly.");
	}

	_change_worker_proxy_state(worker_proxy_res, SID_WRK_STATE_EXITED);

	/*
	 * NOTE: We have set lower priority for this _on_worker_proxy_child_event handler
	 * for us to be able to process all remaining events (e.g. data reception on channels)
	 * before we destroy worker_proxy_res here.
	 *
	 * The approach with setting lower priority for this handler has a downside. Since it
	 * also sets the worker proxy state, then if there are other remaining events with higher
	 * priority, the worke proxy state setting is delayed.
	 *
	 * If this appears as an issue in the future, setting the state and destroying the
	 * worker proxy needs to be separated.
	 */

	if (sid_res_match(worker_proxy_res, &sid_res_type_wrk_prx_with_ev_loop, NULL))
		sid_res_ev_loop_exit(worker_proxy_res);

	(void) sid_res_unref(worker_proxy_res);
	return 0;
}

/*
static int _on_worker_proxy_idle_timeout_event(sid_res_ev_src_t *es, uint64_t usec, void *data)
{
        sid_res_t *worker_proxy_res = data;

        log_debug(ID(worker_proxy_res), "Idle timeout expired.");
        return _make_worker_exit(worker_proxy_res);
}
*/

static int _on_worker_signal_event(sid_res_ev_src_t *es, const struct signalfd_siginfo *si, void *arg)
{
	sid_res_t     *res = arg;
	struct worker *worker;

	sid_res_log_debug(res, "Received signal %d (SIG%s) from %d.", si->ssi_signo, sigabbrev_np(si->ssi_signo), si->ssi_pid);

	switch (si->ssi_signo) {
		case SIGTERM:
		case SIGINT:
			sid_res_ev_loop_exit(res);
			break;
		case SIGUSR1:
			worker = sid_res_get_data(res);
			if (worker != NULL)
				worker->parent_exited = 1;
			break;
		default:
			break;
	};

	return 0;
}

static int _on_worker_proxy_timeout_event(sid_res_ev_src_t *es, uint64_t usec, void *data)
{
	sid_res_t           *worker_proxy_res = data;
	struct worker_proxy *worker_proxy     = sid_res_get_data(worker_proxy_res);
	int                  signum;
	int                  r = 0;

	_change_worker_proxy_state(worker_proxy_res, SID_WRK_STATE_TIMED_OUT);

	if ((signum = worker_proxy->timeout_spec.signum)) {
		sid_res_log_debug(worker_proxy_res, "Sending signal %d (SIG%s)) to worker process.", signum, sigabbrev_np(signum));

		if ((r = kill(worker_proxy->pid, signum)) < 0)
			sid_res_log_error_errno(worker_proxy_res,
			                        errno,
			                        "Failed to send signal %d (SIG%s) to worker process.",
			                        signum,
			                        sigabbrev_np(signum));
	}

	return r;
}

static int _init_worker_proxy(sid_res_t *worker_proxy_res, const void *kickstart_data, void **data)
{
	const struct worker_kickstart *kickstart    = kickstart_data;
	struct worker_proxy           *worker_proxy = NULL;

	if (!(worker_proxy = mem_zalloc(sizeof(*worker_proxy)))) {
		sid_res_log_error(worker_proxy_res, "Failed to allocate worker_proxy structure.");
		goto fail;
	}

	worker_proxy->pid           = kickstart->pid;
	worker_proxy->type          = kickstart->type;
	worker_proxy->state         = SID_WRK_STATE_NEW;
	worker_proxy->channels      = kickstart->channels;
	worker_proxy->channel_count = kickstart->channel_count;
	worker_proxy->timeout_spec  = kickstart->timeout_spec;
	worker_proxy->arg           = kickstart->arg;

	if (sid_res_ev_create_child(worker_proxy_res,
	                            NULL,
	                            worker_proxy->pid,
	                            WEXITED,
	                            _on_worker_proxy_child_event,
	                            1,
	                            "worker process monitor",
	                            worker_proxy_res) < 0) {
		sid_res_log_error(worker_proxy_res, "Failed to register worker process monitoring in worker proxy.");
		goto fail;
	}

	if (_setup_channels(worker_proxy_res, kickstart->type, kickstart->channels, kickstart->channel_count) < 0)
		goto fail;

	if (kickstart->timeout_spec.usec && sid_res_ev_create_time(worker_proxy_res,
	                                                           &worker_proxy->exec_timeout_es,
	                                                           CLOCK_MONOTONIC,
	                                                           SID_RES_POS_REL,
	                                                           kickstart->timeout_spec.usec,
	                                                           0,
	                                                           _on_worker_proxy_timeout_event,
	                                                           0,
	                                                           "timeout",
	                                                           worker_proxy_res) < 0) {
		sid_res_log_error(worker_proxy_res, "Failed to create timeout event.");
		goto fail;
	}

	*data = worker_proxy;
	return 0;
fail:
	free(worker_proxy);
	return -1;
}

static int _destroy_worker_proxy(sid_res_t *worker_proxy_res)
{
	struct worker_proxy *worker_proxy = sid_res_get_data(worker_proxy_res);

	_destroy_channels(worker_proxy->channels, worker_proxy->channel_count);
	free(worker_proxy);

	return 0;
}

static int _init_worker(sid_res_t *worker_res, const void *kickstart_data, void **data)
{
	const struct worker_kickstart *kickstart = kickstart_data;
	struct worker                 *worker    = NULL;
	sigset_t                       mask;

	sigemptyset(&mask);
	sigaddset(&mask, SIGTERM);
	sigaddset(&mask, SIGINT);
	sigaddset(&mask, SIGUSR1);

	if (!(worker = mem_zalloc(sizeof(*worker)))) {
		sid_res_log_error(worker_res, "Failed to allocate new worker structure.");
		goto fail;
	}

	worker->channel_specs = kickstart->channel_specs;
	worker->channels      = kickstart->channels;
	worker->channel_count = kickstart->channel_count;
	worker->parent_exited = 0;
	worker->arg           = kickstart->arg;

	if (sid_res_ev_create_signal(worker_res, NULL, mask, _on_worker_signal_event, 0, "worker_signal_handler", worker_res) < 0) {
		sid_res_log_error(worker_res, "Failed to create signal handlers.");
		goto fail;
	}

	if (_setup_channels(worker_res, kickstart->type, kickstart->channels, kickstart->channel_count) < 0)
		goto fail;

	*data = worker;
	return 0;
fail:
	free(worker);
	return -1;
}

static int _destroy_worker(sid_res_t *worker_res)
{
	struct worker *worker = sid_res_get_data(worker_res);

	_destroy_channels(worker->channels, worker->channel_count);
	free(worker->channel_specs);
	free(worker);

	return 0;
}

static int _set_channel_specs(struct worker_control *worker_control, const struct sid_wrk_chan_spec *channel_specs)
{
	const struct sid_wrk_chan_spec *spec;
	unsigned                        spec_count;
	size_t                          ids_size;
	char                           *p;
	unsigned                        i;
	int                             r;

	for (spec = channel_specs, ids_size = 0, spec_count = 0; spec->wire.type != SID_WRK_WIRE_NONE; spec++) {
		if (!spec->id || !*spec->id) {
			r = -EINVAL;
			goto fail;
		}

		ids_size += strlen(spec->id) + 1; /* +1 to include '\0' at the end of each string! */
		spec_count++;
	}

	/* Allocate overall memory to keep deep copy of all specs, including each spec.id. */
	if (!(worker_control->channel_specs = malloc(spec_count * sizeof(struct sid_wrk_chan_spec) + ids_size))) {
		r = -ENOMEM;
		goto fail;
	}

	for (i = 0, p = (char *) worker_control->channel_specs + spec_count * sizeof(struct sid_wrk_chan_spec); i < spec_count;
	     i++) {
		worker_control->channel_specs[i]    = channel_specs[i];
		worker_control->channel_specs[i].id = p;
		p                                   = stpcpy(p, channel_specs[i].id) + 1;
	}

	worker_control->channel_spec_count = spec_count;
	return 0;
fail:
	worker_control->channel_specs = mem_freen(worker_control->channel_specs);
	return r;
}

static int _init_worker_control(sid_res_t *worker_control_res, const void *kickstart_data, void **data)
{
	const struct sid_wrk_ctl_res_params *params = kickstart_data;
	struct worker_control               *worker_control;
	int                                  r;

	if (!(worker_control = mem_zalloc(sizeof(*worker_control)))) {
		sid_res_log_error(worker_control_res, "Failed to allocate memory for worker control structure.");
		goto fail;
	}

	if ((r = _set_channel_specs(worker_control, params->channel_specs)) < 0) {
		sid_res_log_error_errno(worker_control_res, r, "Failed to set channel specs while initializing worker control.");
		goto fail;
	}

	worker_control->worker_type  = params->worker_type;
	worker_control->init_cb_spec = params->init_cb_spec;
	worker_control->timeout_spec = params->timeout_spec;

	*data                        = worker_control;
	return 0;
fail:
	free(worker_control);
	return -1;
}

static int _destroy_worker_control(sid_res_t *worker_control_res)
{
	struct worker_control *worker_control = sid_res_get_data(worker_control_res);

	if (worker_control->worker_init.channels)
		_destroy_channels(worker_control->worker_init.channels, worker_control->channel_spec_count);
	free(worker_control->worker_init.id);
	free(worker_control->worker_init.ext_argv);
	free(worker_control->worker_init.ext_envp);
	free(worker_control->channel_specs);
	free(worker_control);
	return 0;
}

#define WORKER_PROXY_NAME       "worker-proxy"
#define WORKER_PROXY_SHORT_NAME "wrp"
#define WORKER_PROXY_DESCRIPTION                                                                                                   \
	"Resource under worker-control management providing worker representation "                                                \
	"on parent process side ('proxy') and containting communication endpoints "                                                \
	"for worker-proxy <--> worker channels."

const sid_res_type_t sid_res_type_wrk_prx = {
	.name        = WORKER_PROXY_NAME,
	.short_name  = WORKER_PROXY_SHORT_NAME,
	.description = WORKER_PROXY_DESCRIPTION,
	.init        = _init_worker_proxy,
	.destroy     = _destroy_worker_proxy,
};

const sid_res_type_t sid_res_type_wrk_prx_with_ev_loop = {
	.name            = WORKER_PROXY_NAME,
	.short_name      = WORKER_PROXY_SHORT_NAME,
	.description     = WORKER_PROXY_DESCRIPTION,
	.init            = _init_worker_proxy,
	.destroy         = _destroy_worker_proxy,
	.with_event_loop = 1,
};

const sid_res_type_t sid_res_type_wrk = {
	.name            = "worker",
	.short_name      = "wrk",
	.description     = "Top-level resource in a worker process spawned by worker-control "
			   "resource and containting worker communication endpoints for "
			   "worker <--> worker-proxy channels.",
	.init            = _init_worker,
	.destroy         = _destroy_worker,
	.with_event_loop = 1,
};

const sid_res_type_t sid_res_type_wrk_ctl = {
	.name        = "worker-control",
	.short_name  = "wcl",
	.description = "Resource providing capabilities to spawn worker processes "
		       "and setting up communication channels with workers.",
	.init        = _init_worker_control,
	.destroy     = _destroy_worker_control,
};
