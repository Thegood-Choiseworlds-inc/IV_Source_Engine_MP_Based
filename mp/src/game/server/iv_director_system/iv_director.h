#ifndef _INCLUDED_IV_DIRECTOR_H
#define _INCLUDED_IV_DIRECTOR_H
#ifdef _WIN32
#pragma once
#endif

#include "igamesystem.h"

// A players's intensity level (how much tension/excitement they're experiencing)
class CIVPlayer_Intensity
{
public:
	CIVPlayer_Intensity();

	enum IntensityType
	{
		NONE,
		MILD,
		MODERATE,
		HIGH,
		EXTREME,
		MAXIMUM			// peg the intensity meter
	};

	void  Update( float fDeltaTime );
	void  Increase( IntensityType i );
	float GetCurrent( void ) const;				// return value from 0 (none) to 1 (extreme)
	void  InhibitDecay( float duration );		// inhibit intensity decay for given duration
	void  Reset() { m_flIntensity = 0.0f; }

protected:
	float m_flIntensity;							// current "intensity" from 0 to 1
	CountdownTimer m_decayInhibitTimer;
};

inline void CIVPlayer_Intensity::InhibitDecay(float duration)
{
	if ( m_decayInhibitTimer.GetRemainingTime() < duration )
	{
		m_decayInhibitTimer.Start( duration );
	}
}

inline float CIVPlayer_Intensity::GetCurrent(void) const
{
	return m_flIntensity;
}

class CBasePlayer;

class CIV_Director : public CAutoGameSystemPerFrame
{
public:
	CIV_Director();
	~CIV_Director();

	virtual bool Init();
	virtual void Shutdown();

	virtual void LevelInitPreEntity();
	virtual void LevelInitPostEntity();
	virtual void FrameUpdatePreEntityThink();
	virtual void FrameUpdatePostEntityThink();

	void Event_NPCKilled( CBaseEntity *pNPC, const CTakeDamageInfo &info );
	void PlayerTookDamage(CBasePlayer *pPlayer, const CTakeDamageInfo &info, bool bFriendlyFire);

	// Spawning hordes of NPC's
	void OnHordeFinishedSpawning();

	// intensity access
	float GetMaxIntensity();

	void StartFinale();
	void SetHordesEnabled( bool bHordes ) { m_bHordesEnabled = bHordes; }
	void SetWanderersEnabled( bool bWanderers ) { m_bWanderersEnabled = bWanderers; }

	void SetSpawnTableType(int table_type_index);

protected:
	void UpdateHorde();
	void UpdateIntensity();
	void UpdateSpawningState();
	void UpdateWanderers();

private:
	IntervalTimer m_IntensityUpdateTimer;
	CountdownTimer m_HordeTimer;			// director attempts to spawn a horde when this timer is up
	bool m_bHordeInProgress;

	bool m_bInitialWait;
	bool m_bSpawningNPCs;
	bool m_bReachedIntensityPeak;
	CountdownTimer m_SustainTimer;
	float m_fTimeBetweenNPCs;
	CountdownTimer m_NPCSpawnTimer;

	// players are about to escape, throw everything at them
	bool m_bFinale;
	bool m_bWanderersEnabled;
	bool m_bHordesEnabled;
	bool m_bFiredEscapeRoom;
	bool m_bDirectorControlsSpawners;
};

CIV_Director* IVDirector();

#endif // _INCLUDED_IV_DIRECTOR_H