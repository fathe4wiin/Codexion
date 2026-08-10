/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   logger.c                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: student <student@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/08 16:00:00 by student           #+#    #+#             */
/*   Updated: 2026/08/08 16:00:00 by student          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"

const char	*state_str(t_state st)
{
	if (st == ST_TAKE)
		return ("has taken a dongle");
	if (st == ST_COMPILE)
		return ("is compiling");
	if (st == ST_DEBUG)
		return ("is debugging");
	if (st == ST_REFACTOR)
		return ("is refactoring");
	if (st == ST_BURNOUT)
		return ("burned out");
	return ("");
}

void	log_msg(t_sim *sim, int id, t_state st)
{
	if (!sim)
		return ;
	pthread_mutex_lock(&sim->log_mtx);
	if (!is_stopped(sim) || st == ST_BURNOUT)
		printf("%lld %d %s\n", elapsed_ms(sim), id, state_str(st));
	pthread_mutex_unlock(&sim->log_mtx);
}

void	log_take(t_sim *sim, int id)
{
	log_msg(sim, id, ST_TAKE);
}

void	log_burnout(t_sim *sim, int id)
{
	log_msg(sim, id, ST_BURNOUT);
}
