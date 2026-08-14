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

int	try_pair_once(t_coder *c, t_dongle *a, t_dongle *b)
{
	if (join_pair_queues(a, b, c))
		return (1);
	return (claim_or_mark(a, b, c));
}

int	wait_for_stop(t_sim *sim)
{
	pthread_mutex_lock(&sim->table_mtx);
	while (!sim->stopped)
		pthread_cond_wait(&sim->table_cv, &sim->table_mtx);
	pthread_mutex_unlock(&sim->table_mtx);
	return (1);
}

int	take_two_dongles(t_coder *c)
{
	t_sim	*sim;
	int		st;

	sim = c->sim;
	if (c->left == c->right)
		return (wait_for_stop(sim));
	st = 1;
	pthread_mutex_lock(&sim->table_mtx);
	while (st && !sim->stopped)
	{
		st = try_pair_once(c, c->left, c->right);
		if (st == 2)
			pthread_cond_broadcast(&sim->table_cv);
		if (st)
			table_wait(sim, pair_wake_at(c->left, c->right));
	}
	if (st)
		leave_pair(c->left, c->right, c);
	pthread_mutex_unlock(&sim->table_mtx);
	if (st)
		return (1);
	log_take(sim, c->id);
	log_take(sim, c->id);
	return (0);
}
