/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   heap.c                                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: bfathi <bfathi@student.1337.ma>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/13 19:00:00 by bfathi            #+#    #+#             */
/*   Updated: 2026/08/13 19:00:00 by bfathi           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"

int	heap_init(t_heap *h, t_sched sched)
{
	if (!h)
		return (1);
	h->size = 0;
	h->sched = sched;
	return (0);
}

int	req_better(t_req *a, t_req *b, t_sched sched)
{
	if (!a || !b)
		return (0);
	if (sched == CX_FIFO)
	{
		if (a->arrival != b->arrival)
			return (a->arrival < b->arrival);
		return (a->coder_id < b->coder_id);
	}
	if (sched == CX_EDF)
	{
		if (a->deadline != b->deadline)
			return (a->deadline < b->deadline);
		return (a->coder_id < b->coder_id);
	}
	return (0);
}

int	heap_push(t_heap *h, t_req req)
{
	if (!h || h->size >= HEAP_CAP)
		return (1);
	if (h->size == 0)
	{
		h->data[0] = req;
		h->size = 1;
		return (0);
	}
	if (req_better(&req, &h->data[0], h->sched))
	{
		h->data[1] = h->data[0];
		h->data[0] = req;
	}
	else
		h->data[1] = req;
	h->size = 2;
	return (0);
}

int	heap_find(t_heap *h, int coder_id)
{
	if (!h)
		return (-1);
	if (h->size > 0 && h->data[0].coder_id == coder_id)
		return (0);
	if (h->size > 1 && h->data[1].coder_id == coder_id)
		return (1);
	return (-1);
}

int	heap_remove_id(t_heap *h, int coder_id)
{
	int	i;

	i = heap_find(h, coder_id);
	if (i < 0)
		return (1);
	if (i == 0 && h->size == 2)
		h->data[0] = h->data[1];
	h->size--;
	return (0);
}
