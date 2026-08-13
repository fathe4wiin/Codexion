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
	if (!d)
		return (0);
	return (get_time_ms() >= d->ready_at);
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

void	waiter_set_blocked(t_dongle *d, t_coder *c, int v)
{
	int	i;

	if (!d || !c)
		return ;
	i = heap_find(&d->queue, c->id);
	if (i >= 0)
		d->queue.data[i].blocked = v;
}
