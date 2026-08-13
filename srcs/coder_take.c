/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   coder_take.c                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: bfathi <bfathi@student.1337.ma>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/12 21:20:00 by bfathi            #+#    #+#             */
/*   Updated: 2026/08/13 18:40:00 by bfathi           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"

void	build_req(t_req *req, t_coder *c)
{
	req->coder_id = c->id;
	req->blocked = 0;
	req->arrival = get_time_ms();
	req->deadline = get_deadline(c);
}

int	join_pair_queues(t_dongle *a, t_dongle *b, t_coder *c)
{
	t_req	req;

	build_req(&req, c);
	if (ensure_queued(a, c, &req))
		return (1);
	if (ensure_queued(b, c, &req))
	{
		dequeue_waiter(a, c);
		return (1);
	}
	return (0);
}

void	wait_pair_tick(t_dongle *a, t_dongle *b)
{
	t_dongle		*p;
	struct timespec	ts;
	long long		until;

	p = a;
	if (b->id < a->id)
		p = b;
	until = get_time_ms() + 1;
	ts.tv_sec = (time_t)(until / 1000);
	ts.tv_nsec = (long)((until % 1000) * 1000000L);
	pthread_mutex_lock(&p->mtx);
	pthread_cond_timedwait(&p->cv, &p->mtx, &ts);
	pthread_mutex_unlock(&p->mtx);
}

int	try_pair_once(t_coder *c, t_dongle *a, t_dongle *b)
{
	int	st;

	lock_pair(a, b);
	if (is_stopped(c->sim))
	{
		leave_pair(a, b, c);
		unlock_pair(a, b);
		return (2);
	}
	if (join_pair_queues(a, b, c))
	{
		unlock_pair(a, b);
		return (1);
	}
	st = claim_or_mark(a, b, c);
	unlock_pair(a, b);
	if (st == 0)
	{
		log_take(c->sim, c->id);
		log_take(c->sim, c->id);
	}
	return (st);
}

int	take_two_dongles(t_coder *c)
{
	t_dongle	*a;
	t_dongle	*b;
	int			st;

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
		st = try_pair_once(c, a, b);
		if (st == 2)
			return (1);
		if (st == 0)
			return (0);
		wait_pair_tick(a, b);
	}
	return (1);
}
