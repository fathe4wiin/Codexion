/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   heap_sift.c                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: bfathi <bfathi@student.1337.ma>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/08 16:00:00 by bfathi            #+#    #+#             */
/*   Updated: 2026/08/10 19:34:53 by bfathi           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"

void	heap_swap(t_req *a, t_req *b)
{
	t_req	tmp;

	tmp = *a;
	*a = *b;
	*b = tmp;
}

int	req_better(t_heap *h, t_req *a, t_req *b)
{
	if (!h || !a || !b)
		return (0);
	if (h->sched == CX_EDF && a->deadline != b->deadline)
		return (a->deadline < b->deadline);
	if (a->arrival != b->arrival)
		return (a->arrival < b->arrival);
	return (a->coder_id < b->coder_id);
}

void	heap_sift_up(t_heap *h, int i)
{
	int	p;

	while (i > 0)
	{
		p = (i - 1) / 2;
		if (!req_better(h, &h->data[i], &h->data[p]))
			return ;
		heap_swap(&h->data[i], &h->data[p]);
		i = p;
	}
}

void	heap_sift_down(t_heap *h, int i)
{
	int	l;
	int	r;
	int	best;

	while (1)
	{
		l = 2 * i + 1;
		r = 2 * i + 2;
		best = i;
		if (l < h->size && req_better(h, &h->data[l], &h->data[best]))
			best = l;
		if (r < h->size && req_better(h, &h->data[r], &h->data[best]))
			best = r;
		if (best == i)
			return ;
		heap_swap(&h->data[i], &h->data[best]);
		i = best;
	}
}
