//========= Created by Ivan Suvorov and THS inc 2024. ============//
//
// Purpose: Material System Global Parms In Game Control.
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


//------------------------------------------------------------------------------
// Purpose : Material Global Parms entity
//------------------------------------------------------------------------------
class C_MatGlobalParmsControl : public C_BaseEntity
{
public:
	DECLARE_CLASS(C_MatGlobalParmsControl, C_BaseEntity);

	DECLARE_CLIENTCLASS();

	void OnDataChanged(DataUpdateType_t updateType);
	bool ShouldDraw();

private:
	bool m_bForceEnablePhongOnBrushes;
	bool m_bForceEnableAnisotopicReflection;
};

IMPLEMENT_CLIENTCLASS_DT(C_MatGlobalParmsControl, DT_MatGlobalParmsControl, CMatGlobalParmsControl)
RecvPropBool(RECVINFO(m_bForceEnablePhongOnBrushes)),
RecvPropBool(RECVINFO(m_bForceEnableAnisotopicReflection)),
END_RECV_TABLE()


//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
void C_MatGlobalParmsControl::OnDataChanged(DataUpdateType_t updateType)
{
	ConVarRef temp_convar_force_lightmapped_phong("mat_force_lightmapped_phong");
	ConVarRef temp_convar_force_envmap_anisotopy("mat_force_envmap_anisotropy");

	if (!temp_convar_force_lightmapped_phong.IsValid() || !temp_convar_force_envmap_anisotopy.IsValid())
	{
		Warning("Invalid Material ConVars!!! Aborting Global Parms Change!!!\n");
		return;
	}

	temp_convar_force_lightmapped_phong.SetValue(m_bForceEnablePhongOnBrushes);
	temp_convar_force_envmap_anisotopy.SetValue(m_bForceEnableAnisotopicReflection);
}

//------------------------------------------------------------------------------
// We don't draw...
//------------------------------------------------------------------------------
bool C_MatGlobalParmsControl::ShouldDraw()
{
	return false;
}
