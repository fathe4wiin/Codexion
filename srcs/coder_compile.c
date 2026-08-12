/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   coder_compile.c                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: bfathi <bfathi@student.1337.ma>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/08 16:00:00 by bfathi            #+#    #+#             */
/*   Updated: 2026/08/12 21:00:00 by bfathi           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"

void	bump_compile(t_coder *c)
{
	if (!c)
		return ;
	c->n_compiled++;
}

int	do_compile(t_coder *c)
{
	if (!c || !c->sim || is_stopped(c->sim))
		return (1);
	pthread_mutex_lock(&c->state_mtx);
	c->last_compile = get_time_ms();
	log_msg(c->sim, c->id, ST_COMPILE);
	pthread_mutex_unlock(&c->state_mtx);
	return (act_sleep(c->sim, c->sim->cfg.t_compile));
}

void	release_two_dongles(t_coder *c)
{
	if (!c)
		return ;
	if (c->left == c->right)
	{
		release_dongle(c->left, c);
		return ;
	}
	if (dongle_first(c) == 0)
	{
		release_dongle(c->right, c);
		release_dongle(c->left, c);
	}
	else
	{
		release_dongle(c->left, c);
		release_dongle(c->right, c);
	}
}

static void	yield_if_blocking(t_dongle *d, t_dongle *other, t_coder *c)
{
	if (!can_take(d, c) || can_take(other, c))
		return ;
	dequeue_waiter(d, c);
	signal_waiter(d);
}

static int	claim_pair(t_coder *c, t_dongle *a, t_dongle *b)
{
	t_req	tmp;

	if (!can_take(a, c) || !can_take(b, c))
		return (1);
	if (a->queue.size > 0)
		heap_pop(&a->queue, &tmp);
	if (b->queue.size > 0)
		heap_pop(&b->queue, &tmp);
	a->holder = c->id;
	b->holder = c->id;
	return (0);
}

static int	try_take_pair(t_coder *c, t_dongle *a, t_dongle *b)
{
	if (ensure_queued(a, c) || ensure_queued(b, c))
		return (1);
	if (!claim_pair(c, a, b))
		return (0);
	yield_if_blocking(a, b, c);
	yield_if_blocking(b, a, c);
	return (1);
}

static void	abort_pair_wait(t_coder *c, t_dongle *a, t_dongle *b)
{
	dequeue_waiter(a, c);
	dequeue_waiter(b, c);
	signal_waiter(a);
	signal_waiter(b);
}

int	take_two_dongles(t_coder *c)
{
	t_dongle	*a;
	t_dongle	*b;

	if (!c || !c->sim)
		return (1);
	if (c->left == c->right)
	{
		while (!is_stopped(c->sim))
			usleep(1000);
		return (1);
	}
	a = c->left;
	b = c->right;
	while (!is_stopped(c->sim))
	{
		lock_dongle_pair(a, b);
		if (is_stopped(c->sim))
		{
			abort_pair_wait(c, a, b);
			unlock_dongle_pair(a, b);
			return (1);
		}
		if (!try_take_pair(c, a, b))
		{
			unlock_dongle_pair(a, b);
			log_take(c->sim, c->id);
			log_take(c->sim, c->id);
			return (0);
		}
		unlock_dongle_pair(a, b);
		usleep(500);
	}
	lock_dongle_pair(a, b);
	abort_pair_wait(c, a, b);
	unlock_dongle_pair(a, b);
	return (1);
}

int	compile_cycle(t_coder *c)
{
	if (!c || coder_should_exit(c))
		return (1);
	if (take_two_dongles(c))
		return (1);
	if (do_compile(c))
	{
		release_two_dongles(c);
		return (1);
	}
	release_two_dongles(c);
	bump_compile(c);
	return (0);
}
