# Findings (MW rain)

## IDA addresses (from ida_mcp_server)
- sub_6DE300 (display frame render): 0x006DE300
- Rain tick callsite inside sub_6DE300: 0x006DF545 (calls sub_758100)
- Rain::Render: 0x0074ACC0
- Rain::Render3D: 0x0074A5D0
- Rain::Tick/Update wrapper: 0x00758100
- g_RainEnable: 0x00901810
- g_ParticleSystemEnable: 0x009017EC
- Rain instance ptr: 0x009196B8 (dword_9196B8)
- Render__4Rain uses Rain+0x284 (must be non-null) and Rain+0x288 (eView)
- eView + 0x44 used in sub_74A070/sub_74A160 (likely viewPlat pointer)
- eViews array base: 0x009195E0
- eViews_0 ptr: 0x009196B4
- Alt-matrix flag ptr: 0x00982CB4 (inst[0x50])

## PreFEng (MW) precipitation globals
- PRECIPITATION_ENABLE_ADDR  0x008F86E4
- PRECIPITATION_RENDER_ADDR  0x00904AD0
- PRECIP_RAINX_ADDR          0x00904A20
- PRECIP_RAINY_ADDR          0x00904A24
- PRECIP_RAINZ_ADDR          0x00904A28
- PRECIP_RAINZCONSTANT_ADDR  0x00904A2C
- PRECIP_BOUNDX_ADDR         0x00904A60
- PRECIP_BOUNDY_ADDR         0x00904A64
- PRECIP_BOUNDZ_ADDR         0x00904A68
- PRECIP_AHEADX_ADDR         0x00904A6C
- PRECIP_AHEADY_ADDR         0x009B0A34
- PRECIP_AHEADZ_ADDR         0x009B0A38
- PRECIP_RAINWINDEFF_ADDR    0x00904A80
- PRECIP_RAINRADIUSX_ADDR    0x00904A90
- PRECIP_RAINRADIUSY_ADDR    0x00904A94
- PRECIP_RAINRADIUSZ_ADDR    0x00904A98
- PRECIP_DRIVEFACTOR_ADDR    0x00904ACC
- PRECIP_RAINPERCENT_ADDR    0x00904AD8
- PRECIP_FOGPERCENT_ADDR     0x009B0A40
- PRECIP_RAININTHEHEADLIGHTS_ADDR 0x008F2924
- PRECIP_WINDANG_ADDR        0x009B0A50
- PRECIP_SWAYMAX_ADDR        0x00904AE8
- PRECIP_MAXWINDEFF_ADDR     0x00904AF0
- PRECIP_PREVAILINGMULT_ADDR 0x00904AF4
- PRECIP_ONSCREEN_DRIPSPEED_ADDR        0x00904B2C
- PRECIP_ONSCREEN_SPEEDMOD_ADDR         0x00904B30
- PRECIP_ONSCREEN_DROPSHAPESPEEDCHANGE_ADDR 0x00904B34
- PRECIP_BASEDAMPNESS_ADDR   0x00904B38
- PRECIP_RAINRATEOFCHANGE_ADDR 0x00904AC4
- PRECIP_CLOUDSRATEOFCHANGE_ADDR 0x00904AC8
- PRECIP_CAMERAMOD_ADDR      0x00904AE0
- FOG_CTRLOVERRIDE_ADDR      0x009B0A70

## Current symptoms
- 3D rain not visible (sometimes a static dot).
- HookedSetTransform VIEW/PROJ fires, but render has no visible 3D output.

## Suggestions
1) Force MW native precipitation globals:
   - Set PRECIPITATION_ENABLE_ADDR = 1 and PRECIPITATION_RENDER_ADDR = 1.
   - Update PRECIP_RAINPERCENT_ADDR and PRECIP_FOGPERCENT_ADDR from config.
   - Optionally set PRECIP_RAINX/Y/Z from camera each frame.

2) Ensure native rain tick runs in correct render context:
   - sub_6DE300 already calls sub_758100 at 0x006DF545 when g_RainEnable && g_ParticleSystemEnable.
   - If needed, hook callsite 0x006DF545 to a wrapper that forces flags then calls sub_758100 once.

3) Avoid double render:
   - Replace callsite rather than calling from Present/Update.

## Redis keys stored
Hash: nfsmw:rain
- g_RainEnable = 0x00901810
- g_ParticleSystemEnable = 0x009017EC
- rain_callsite_sub_6DE300 = 0x006DF545
- RainRender3D_MW = 0x0074A5D0
- RainRender_MW = 0x0074ACC0
- RainTick_MW = 0x00758100
- RainInstancePtr_MW = 0x009196B8
- EViewArrayBase_MW = 0x009195E0
- EViewCurrentPtr_MW = 0x009196B4
- EViewAltMatrixFlagPtr_MW = 0x00982CB4
- EViewCameraOffset_MW = 0x38
- EViewCameraParamsOffset_MW = 0x40
- EViewMatrixOffsets_MW = 0x40,0x80,0xC0,0x100
- BuildRenderViewAddr_MW = 0x006CEEE0
- BuildRenderMatrixAddr_MW = 0x006C8000
- BuildViewMatrixAddr_MW = 0x006CF400
- CreateLookAtAddr_MW = 0x006CF0A0

## Notes
- Forcing g_RoadReflectionEnable (0x009017D4) made roads look snowy; keep for future snow mod work. Reverted for rain flow. 
