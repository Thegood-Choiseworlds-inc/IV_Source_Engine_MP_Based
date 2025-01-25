#include "cbase.h"
#include "iv_director.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

extern ConVar iv_director_debug;

ConVar iv_intensity_scale("iv_intensity_scale", "0.25", FCVAR_CHEAT, "Scales intensity increases for Players");
ConVar iv_intensity_decay_time("iv_intensity_decay_time", "20", FCVAR_CHEAT, "Seconds to decay full intensity to zero");
ConVar iv_intensity_inhibit_delay("iv_intensity_inhibit_delay", "3.5", FCVAR_CHEAT, "Seconds before intensity starts to decay after an increase");
ConVar iv_intensity_far_range("iv_intensity_far_range", "200", FCVAR_CHEAT, "Enemies killed past this distance will only slightly increase intensity");

CIVPlayer_Intensity::CIVPlayer_Intensity()
{
	m_flIntensity = 0.0f;
}

void CIVPlayer_Intensity::Increase(IntensityType i)
{
	float value = 0.0f;

	switch( i )
	{
	case MILD:
		value = 0.05f;
		break;

	case MODERATE:
		value = 0.2f;
		break;

	case HIGH:
		value = 0.5f;
		break;

	case EXTREME:
		value = 1.0f;
		break;

	case MAXIMUM:
		value = 999999.9f;		// force intensity to max
		break;
	}

	m_flIntensity += iv_intensity_scale.GetFloat() * value;
	if (m_flIntensity > 1.0f)
	{
		m_flIntensity = 1.0f;
	}

	// don't decay immediately
	InhibitDecay( iv_intensity_inhibit_delay.GetFloat() );
}

void CIVPlayer_Intensity::Update(float fDeltaTime)
{
	if (m_decayInhibitTimer.IsElapsed())
	{
		m_flIntensity -= fDeltaTime / iv_intensity_decay_time.GetFloat();
		if (m_flIntensity < 0.0f)
		{
			m_flIntensity = 0.0f;
		}
	}
}


void CIV_Director::UpdateIntensity()
{
	float fDeltaTime = m_IntensityUpdateTimer.GetElapsedTime();
	m_IntensityUpdateTimer.Start();

	int total_players = gpGlobals->maxClients;

	if (total_players <= 0)
		return;

	for (int i = 1; i <= total_players; i++)
	{
		CBasePlayer *pPlayer = UTIL_PlayerByIndex(i);

		if (!pPlayer)
			continue;

		pPlayer->GetIntensity()->Update(fDeltaTime);

		if (iv_director_debug.GetInt() > 0)
		{
			engine->Con_NPrintf(i + 2, "Player %d Intensity = %f", i, pPlayer->GetIntensity()->GetCurrent());
		}
	}
}


float CIV_Director::GetMaxIntensity()
{
	int total_players = gpGlobals->maxClients;

	if (total_players <= 0)
		return 0.0f;

	float flIntensity = 0;
	for (int i = 1; i <= total_players; i++)
	{
		CBasePlayer *pPlayer = UTIL_PlayerByIndex(i);

		if (!pPlayer)
			continue;

		flIntensity = MAX(flIntensity, pPlayer->GetIntensity()->GetCurrent());
	}

	return flIntensity;
}