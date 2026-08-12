/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   dongle_take.c                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: bfathi <bfathi@student.1337.ma>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/08 16:00:00 by bfathi            #+#    #+#             */
/*   Updated: 2026/08/12 20:56:00 by bfathi           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"

int	dongle_ready(t_dongle *d)
{
	if (!d)
		return (0);
	return (get_time_ms() >= d->ready_at);
}

int	can_take(t_dongle *d, t_coder *c)
{
	t_req	top;

	if (!d || !c || d->holder >= 0 || !dongle_ready(d))
		return (0);
	if (d->queue.size <= 0)
		return (1);
	if (heap_peek(&d->queue, &top))
		return (0);
	return (top.coder_id == c->id);
}

int	acquire_now(t_dongle *d, t_coder *c)
{
	t_req	top;

	if (!can_take(d, c))
		return (1);
	if (d->queue.size > 0 && heap_pop(&d->queue, &top))
		return (1);
	d->holder = c->id;
	return (0);
}

int	try_acquire(t_dongle *d, t_coder *c)
{
	return (!acquire_now(d, c));
}

int	enqueue_waiter(t_dongle *d, t_coder *c)
{
	t_req	req;

	req.coder_id = c->id;
	req.arrival = get_time_ms();
	req.deadline = get_deadline(c);
	return (heap_push(&d->queue, req));
}

int	ensure_queued(t_dongle *d, t_coder *c)
{
	if (!d || !c)
		return (1);
	if (heap_find(&d->queue, c->id) >= 0)
		return (0);
	return (enqueue_waiter(d, c));
}

void	dequeue_waiter(t_dongle *d, t_coder *c)
{
	if (!d || !c)
		return ;
	heap_remove_id(&d->queue, c->id);
}

int	wait_for_grant(t_dongle *d, t_coder *c)
{
	struct timespec	ts;
	long long		ready;

	while (!is_stopped(d->sim) && d->holder != c->id)
	{
		if (try_acquire(d, c))
			return (0);
		ready = d->ready_at;
		if (!dongle_ready(d) && ready > 0)
		{
			ts.tv_sec = ready / 1000;
			ts.tv_nsec = (ready % 1000) * 1000000L;
			pthread_cond_timedwait(&d->cv, &d->mtx, &ts);
		}
		else
			pthread_cond_wait(&d->cv, &d->mtx);
	}
	return (d->holder != c->id);
}

void	lock_dongle_pair(t_dongle *a, t_dongle *b)
{
	if (a->id < b->id)
	{
		pthread_mutex_lock(&a->mtx);
		pthread_mutex_lock(&b->mtx);
	}
	else
	{
		pthread_mutex_lock(&b->mtx);
		pthread_mutex_lock(&a->mtx);
	}
}

void	unlock_dongle_pair(t_dongle *a, t_dongle *b)
{
	if (a->id < b->id)
	{
		pthread_mutex_unlock(&b->mtx);
		pthread_mutex_unlock(&a->mtx);
	}
	else
	{
		pthread_mutex_unlock(&a->mtx);
		pthread_mutex_unlock(&b->mtx);
	}
}

int	take_dongle(t_dongle *d, t_coder *c)
{
	if (!d || !c || !d->sim)
		return (1);
	pthread_mutex_lock(&d->mtx);
	if (is_stopped(d->sim))
	{
		pthread_mutex_unlock(&d->mtx);
		return (1);
	}
	if (!try_acquire(d, c))
	{
		if (enqueue_waiter(d, c) || wait_for_grant(d, c))
		{
			dequeue_waiter(d, c);
			pthread_mutex_unlock(&d->mtx);
			return (1);
		}
	}
	pthread_mutex_unlock(&d->mtx);
	log_take(d->sim, c->id);
	return (0);
}
