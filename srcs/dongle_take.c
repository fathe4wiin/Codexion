/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   dongle_take.c                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: bfathi <bfathi@student.1337.ma>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/08 16:00:00 by bfathi            #+#    #+#             */
/*   Updated: 2026/08/12 21:30:00 by bfathi           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"

int	dongle_ready(t_dongle *d)
{
	if (!d)
		return (0);
	return (get_time_ms() >= d->ready_at);
}

/*
** A request ahead of ours only holds us back while its owner is able to use
** this dongle: a coder waiting for its other dongle marks itself blocked.
*/
int	priority_ok(t_dongle *d, t_coder *c)
{
	t_heap	*h;
	int		self;
	int		i;

	h = &d->queue;
	self = heap_find(h, c->id);
	if (self < 0)
		return (0);
	i = 0;
	while (i < h->size)
	{
		if (i != self && !req_yields(&h->data[i], d->sim)
			&& req_better(h, &h->data[i], &h->data[self]))
			return (0);
		i++;
	}
	return (1);
}

int	usable_dongle(t_dongle *d, t_coder *c)
{
	if (!d || !c)
		return (0);
	if (d->holder >= 0 || !dongle_ready(d))
		return (0);
	return (priority_ok(d, c));
}

int	ensure_queued(t_dongle *d, t_coder *c, t_req *req)
{
	if (!d || !c || !req)
		return (1);
	if (heap_find(&d->queue, c->id) >= 0)
		return (0);
	return (heap_push(&d->queue, *req));
}

void	dequeue_waiter(t_dongle *d, t_coder *c)
{
	if (!d || !c)
		return ;
	heap_remove_id(&d->queue, c->id);
}
