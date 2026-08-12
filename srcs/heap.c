/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   heap.c                                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: bfathi <bfathi@student.1337.ma>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/08 16:00:00 by bfathi            #+#    #+#             */
/*   Updated: 2026/08/10 19:35:24 by bfathi           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"

int	heap_init(t_heap *h, int cap, t_sched sched)
{
	if (!h || cap <= 0)
		return (1);
	h->data = malloc(sizeof(t_req) * (size_t)cap);
	if (!h->data)
		return (1);
	h->size = 0;
	h->cap = cap;
	h->sched = sched;
	return (0);
}

int	heap_push(t_heap *h, t_req req)
{
	if (!h || !h->data || h->size >= h->cap)
		return (1);
	h->data[h->size] = req;
	heap_sift_up(h, h->size);
	h->size++;
	return (0);
}

int	heap_pop(t_heap *h, t_req *out)
{
	if (!h || !out || h->size <= 0)
		return (1);
	*out = h->data[0];
	h->size--;
	if (h->size > 0)
	{
		h->data[0] = h->data[h->size];
		heap_sift_down(h, 0);
	}
	return (0);
}

int	heap_peek(t_heap *h, t_req *out)
{
	if (!h || !out || h->size <= 0)
		return (1);
	*out = h->data[0];
	return (0);
}

int	heap_find(t_heap *h, int coder_id)
{
	int	i;

	if (!h || !h->data)
		return (-1);
	i = 0;
	while (i < h->size)
	{
		if (h->data[i].coder_id == coder_id)
			return (i);
		i++;
	}
	return (-1);
}

int	heap_remove_id(t_heap *h, int coder_id)
{
	int	i;

	i = heap_find(h, coder_id);
	if (!h || i < 0)
		return (1);
	h->size--;
	if (i == h->size)
		return (0);
	h->data[i] = h->data[h->size];
	heap_sift_down(h, i);
	heap_sift_up(h, i);
	return (0);
}

void	heap_destroy(t_heap *h)
{
	if (!h)
		return ;
	free(h->data);
	h->data = NULL;
	h->size = 0;
	h->cap = 0;
}
