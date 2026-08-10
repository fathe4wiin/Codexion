/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   dongle_take.c                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: student <student@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/08 16:00:00 by student           #+#    #+#             */
/*   Updated: 2026/08/08 16:00:00 by student          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"

int	dongle_ready(t_dongle *d)
{
	if (!d)
		return (0);
	return (get_time_ms() >= d->ready_at);
}

int	try_acquire(t_dongle *d, t_coder *c)
{
	t_req	top;

	if (!d || !c || d->holder >= 0 || !dongle_ready(d))
		return (0);
	if (d->queue.size > 0)
	{
		if (heap_peek(&d->queue, &top) || top.coder_id != c->id)
			return (0);
		heap_pop(&d->queue, &top);
	}
	d->holder = c->id;
	return (1);
}

int	enqueue_waiter(t_dongle *d, t_coder *c)
{
	t_req	req;

	req.coder_id = c->id;
	req.arrival = get_time_ms();
	req.deadline = get_deadline(c);
	return (heap_push(&d->queue, req));
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
			pthread_mutex_unlock(&d->mtx);
			return (1);
		}
	}
	pthread_mutex_unlock(&d->mtx);
	log_take(d->sim, c->id);
	return (0);
}
