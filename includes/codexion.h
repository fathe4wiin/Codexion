/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   codexion.h                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: bfathi <bfathi@student.1337.ma>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/04 17:00:00 by bfathi            #+#    #+#             */
/*   Updated: 2026/08/10 19:35:40 by bfathi           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef CODEXION_H
# define CODEXION_H

# include <pthread.h>
# include <sys/time.h>
# include <stdlib.h>
# include <stdio.h>
# include <string.h>
# include <unistd.h>
# include <limits.h>

typedef enum e_sched
{
	CX_FIFO,
	CX_EDF
}	t_sched;

typedef enum e_state
{
	ST_TAKE,
	ST_COMPILE,
	ST_DEBUG,
	ST_REFACTOR,
	ST_BURNOUT
}	t_state;

typedef struct s_sim	t_sim;
typedef struct s_coder	t_coder;
typedef struct s_dongle	t_dongle;

typedef struct s_config
{
	int			n_coders;
	long long	t_burnout;
	int			t_compile;
	int			t_debug;
	int			t_refactor;
	int			n_compiles;
	long long	dongle_cd;
	t_sched		sched;
}	t_config;

typedef struct s_req
{
	int			coder_id;
	long long	arrival;
	long long	deadline;
}	t_req;

typedef struct s_heap
{
	t_req		*data;
	int			size;
	int			cap;
	t_sched		sched;
}	t_heap;

typedef struct s_dongle
{
	int				id;
	int				holder;
	long long		ready_at;
	pthread_mutex_t	mtx;
	pthread_cond_t	cv;
	t_heap			queue;
	t_sim			*sim;
}	t_dongle;

typedef struct s_coder
{
	int				id;
	int				n_compiled;
	long long		last_compile;
	t_dongle		*left;
	t_dongle		*right;
	pthread_t		thread;
	t_sim			*sim;
}	t_coder;

typedef struct s_sim
{
	t_config		cfg;
	t_coder			*coders;
	t_dongle		*dongles;
	int				n_dongles;
	int				stopped;
	long long		start_ms;
	pthread_mutex_t	stop_mtx;
	pthread_mutex_t	log_mtx;
	pthread_t		monitor;
}	t_sim;

int			parse_args(int ac, char **av, t_config *cfg);
int			parse_pos_int(const char *s, int *out);
int			parse_nn_int(const char *s, int *out);
int			parse_nn_ll(const char *s, long long *out);
int			parse_scheduler(const char *s, t_sched *out);

int			print_error(const char *msg);
int			is_stopped(t_sim *sim);
void		set_stopped(t_sim *sim);
long long	get_deadline(t_coder *c);
long long	get_time_ms(void);
long long	elapsed_ms(t_sim *sim);
int			act_sleep(t_sim *sim, long long ms);
void		sleep_until(long long until_ms);
int			dongle_first(t_coder *c);

int			init_sim(t_sim *sim, t_config *cfg);
int			alloc_sim(t_sim *sim);
int			init_shared(t_sim *sim);
void		stamp_start(t_sim *sim);
int			init_dongles(t_sim *sim);
int			init_one_dongle(t_dongle *d, int id, t_sim *sim);
int			init_coders(t_sim *sim);
void		link_coder_dongles(t_sim *sim);
void		reset_coder(t_coder *c, int id, t_sim *sim);

int			heap_init(t_heap *h, int cap, t_sched sched);
int			heap_push(t_heap *h, t_req req);
int			heap_pop(t_heap *h, t_req *out);
int			heap_peek(t_heap *h, t_req *out);
void		heap_destroy(t_heap *h);
void		heap_sift_up(t_heap *h, int i);
void		heap_sift_down(t_heap *h, int i);
void		heap_swap(t_req *a, t_req *b);
int			req_better(t_heap *h, t_req *a, t_req *b);

int			take_dongle(t_dongle *d, t_coder *c);
int			try_acquire(t_dongle *d, t_coder *c);
int			enqueue_waiter(t_dongle *d, t_coder *c);
int			wait_for_grant(t_dongle *d, t_coder *c);
int			dongle_ready(t_dongle *d);
void		release_dongle(t_dongle *d, t_coder *c);
void		arm_cooldown(t_dongle *d);
void		grant_next(t_dongle *d);
void		signal_waiter(t_dongle *d);

void		*coder_routine(void *arg);
int			coder_loop(t_coder *c);
int			coder_should_exit(t_coder *c);
int			compile_cycle(t_coder *c);
int			take_two_dongles(t_coder *c);
int			do_compile(t_coder *c);
void		release_two_dongles(t_coder *c);
void		bump_compile(t_coder *c);
int			do_debug(t_coder *c);
int			do_refactor(t_coder *c);

void		*monitor_routine(void *arg);
int			check_burnouts(t_sim *sim);
int			check_all_done(t_sim *sim);
void		log_msg(t_sim *sim, int id, t_state st);
void		log_take(t_sim *sim, int id);
void		log_burnout(t_sim *sim, int id);
const char	*state_str(t_state st);

int			start_simulation(t_sim *sim);
int			spawn_coders(t_sim *sim);
int			spawn_monitor(t_sim *sim);
int			join_all(t_sim *sim);

void		cleanup_sim(t_sim *sim);
void		destroy_dongles(t_sim *sim);
void		destroy_shared(t_sim *sim);
void		free_sim(t_sim *sim);

#endif
