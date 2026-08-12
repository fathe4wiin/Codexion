/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   dongle_pair.c                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: bfathi <bfathi@student.1337.ma>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/12 21:20:00 by bfathi            #+#    #+#             */
/*   Updated: 2026/08/12 21:30:00 by bfathi           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"

void	lock_pair(t_dongle *a, t_dongle *b)
{
	if (a->id < b->id)
	{
		pthread_mutex_lock(&a->mtx);
		pthread_mutex_lock(&b->mtx);
		return ;
	}
	pthread_mutex_lock(&b->mtx);
	pthread_mutex_lock(&a->mtx);
}

void	unlock_pair(t_dongle *a, t_dongle *b)
{
	if (a->id < b->id)
	{
		pthread_mutex_unlock(&b->mtx);
		pthread_mutex_unlock(&a->mtx);
		return ;
	}
	pthread_mutex_unlock(&a->mtx);
	pthread_mutex_unlock(&b->mtx);
}

void	claim_pair(t_dongle *a, t_dongle *b, t_coder *c)
{
	dequeue_waiter(a, c);
	dequeue_waiter(b, c);
	a->holder = c->id;
	b->holder = c->id;
}

void	leave_pair(t_dongle *a, t_dongle *b, t_coder *c)
{
	dequeue_waiter(a, c);
	dequeue_waiter(b, c);
	signal_waiter(a);
	signal_waiter(b);
}
