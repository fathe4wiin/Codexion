/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   time_utils.c                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: student <student@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/04 17:00:00 by student           #+#    #+#             */
/*   Updated: 2026/08/08 16:00:00 by student          ###   ########.fr       */
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

void	sleep_until(long long until_ms)
{
	long long	now;

	now = get_time_ms();
	if (until_ms > now)
		usleep((useconds_t)((until_ms - now) * 1000));
}
