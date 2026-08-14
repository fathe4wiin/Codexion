/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   dongle_take.c                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: bfathi <bfathi@student.1337.ma>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/08 16:00:00 by bfathi            #+#    #+#             */
/*   Updated: 2026/08/13 19:00:00 by bfathi           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"

int	dongle_ready(t_dongle *d)
{
	return (get_time_ms() >= d->ready_at);
}

int	ensure_queued(t_dongle *d, t_coder *c, t_req *req)
{
	if (heap_find(&d->queue, c->id) >= 0)
		return (0);
	return (heap_push(&d->queue, *req));
}

void	dequeue_waiter(t_dongle *d, t_coder *c)
{
	heap_remove_id(&d->queue, c->id);
}

/* Returns 1 when the flag really flipped, so callers know to wake peers. */
int	waiter_set_blocked(t_dongle *d, t_coder *c, int v)
{
	int	i;

	i = heap_find(&d->queue, c->id);
	if (i < 0 || d->queue.data[i].blocked == v)
		return (0);
	d->queue.data[i].blocked = v;
	return (1);
}
