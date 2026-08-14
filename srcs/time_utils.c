/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   time_utils.c                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: bfathi <bfathi@student.1337.ma>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/04 17:00:00 by bfathi            #+#    #+#             */
/*   Updated: 2026/08/10 19:35:23 by bfathi           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"

long long	get_time_ms(void)
{
	struct timeval	tv;

	if (gettimeofday(&tv, NULL))
		return (0);
	return ((long long)tv.tv_sec * 1000LL + (long long)tv.tv_usec / 1000LL);
}

long long	elapsed_ms(t_sim *sim)
{
	if (!sim)
		return (0);
	return (get_time_ms() - sim->start_ms);
}

int	act_sleep(t_sim *sim, long long ms)
{
	long long	end;

	if (!sim || ms <= 0)
		return (0);
	end = get_time_ms() + ms;
	while (get_time_ms() < end)
	{
		if (is_stopped(sim))
			return (1);
		usleep(200);
	}
	return (0);
}
