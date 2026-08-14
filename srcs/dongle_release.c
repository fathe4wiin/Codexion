/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   dongle_release.c                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: bfathi <bfathi@student.1337.ma>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/08 16:00:00 by bfathi            #+#    #+#             */
/*   Updated: 2026/08/10 19:34:41 by bfathi           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"

void	release_dongle(t_dongle *d, t_coder *c)
{
	if (d->holder != c->id)
		return ;
	d->holder = -1;
	d->ready_at = get_time_ms() + c->sim->cfg.dongle_cd;
}

void	release_two_dongles(t_coder *c)
{
	t_sim	*sim;

	sim = c->sim;
	pthread_mutex_lock(&sim->table_mtx);
	release_dongle(c->left, c);
	release_dongle(c->right, c);
	pthread_cond_broadcast(&sim->table_cv);
	pthread_mutex_unlock(&sim->table_mtx);
}
