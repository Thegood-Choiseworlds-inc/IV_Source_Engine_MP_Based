//========= Created by Ivan Suvorov and THS inc 2024. ============//
//
// Purpose: Material System Global Parms In Game Control.
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


static bool g_mat_force_phong = false;
static bool g_mat_force_anisotropic = false;

void IV_Force_Reload_All_Materials()
{
	materials->UncacheAllMaterials();
	materials->CacheUsedMaterials();
}


//------------------------------------------------------------------------------
// Purpose : Material Global Parms entity
//------------------------------------------------------------------------------
class C_MatGlobalParmsControl : public C_BaseEntity
{
public:
	DECLARE_CLASS(C_MatGlobalParmsControl, C_BaseEntity);

	DECLARE_CLIENTCLASS();

	C_MatGlobalParmsControl();

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


C_MatGlobalParmsControl::C_MatGlobalParmsControl()
{
	//IV Note: Force Reload All Materials for Revent Changes!!!
	if (g_mat_force_phong || g_mat_force_anisotropic)
	{
		g_mat_force_phong = false;
		g_mat_force_anisotropic = false;
		Warning("Force Revent Back Material Global Parms!!!\n");
		IV_Force_Reload_All_Materials();
	}
}

//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
void C_MatGlobalParmsControl::OnDataChanged(DataUpdateType_t updateType)
{
	ConVarRef temp_convar_force_lightmapped_phong("ivdev_engine_force_lightmapped_phong");
	ConVarRef temp_convar_force_envmap_anisotopy("ivdev_engine_force_envmap_anisotropy");

	if (!temp_convar_force_lightmapped_phong.IsValid() || !temp_convar_force_envmap_anisotopy.IsValid())
	{
		Assert(0);
		Warning("Invalid Material ConVars!!! Aborting Global Parms Change!!!\n");
		return;
	}

	g_mat_force_phong = m_bForceEnablePhongOnBrushes;
	g_mat_force_anisotropic = m_bForceEnableAnisotopicReflection;

	temp_convar_force_lightmapped_phong.SetValue(g_mat_force_phong);
	temp_convar_force_envmap_anisotopy.SetValue(g_mat_force_anisotropic);

	Warning("Force Applying Material Parms for Remake Map!!!\n");
	Warning("Force Applying Material Parms for Remake Map!!!\n");
	Warning("Force Applying Material Parms for Remake Map!!!\n");

	//IV Note: Force Reload All Materials for Apply Changes!!!
	IV_Force_Reload_All_Materials();
}

//------------------------------------------------------------------------------
// We don't draw...
//------------------------------------------------------------------------------
bool C_MatGlobalParmsControl::ShouldDraw()
{
	return false;
}
