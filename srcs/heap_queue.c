/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   heap_queue.c                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: bfathi <bfathi@student.1337.ma>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/12 21:20:00 by bfathi            #+#    #+#             */
/*   Updated: 2026/08/12 21:20:00 by bfathi           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"

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

void	heap_set_blocked(t_heap *h, int coder_id, int v)
{
	int	i;

	i = heap_find(h, coder_id);
	if (i < 0)
		return ;
	h->data[i].blocked = v;
}

int	heap_remove_id(t_heap *h, int coder_id)
{
	int	i;

	if (!h)
		return (1);
	i = heap_find(h, coder_id);
	if (i < 0)
		return (1);
	h->size--;
	if (i == h->size)
		return (0);
	h->data[i] = h->data[h->size];
	heap_sift_down(h, i);
	heap_sift_up(h, i);
	return (0);
}
