//==================================================================
/// XCompUI.cpp
///
/// Created by Davide Pasca - 2022/02/07
/// See the file "license.txt" that comes with this project for
/// copyright info.
//==================================================================

#include "imgui.h"
#include "imgui_stdlib.h"
#include "GTVersions.h"
#include "DLogOut.h"
#include "DateMacro.h"
#include "IMUI_Utils.h"
#include "ConfigWin.h"
#include "DisplayConfigWin.h"
#include "IMUI_AboutDialog.h"
#include "IMUI_Utils.h"
#include "DisplayBase.h"
#include "GraphicsApp.h"
#include "ImageSystem.h"
#ifdef ENABLE_IMGUITEXINSPECT
# include "imgui_tex_inspect.h"
#endif
#include "imgui_internal.h"
#include "RU_IMUITableMaker.h"
#include "XComp.h"
#include "XCompUI.h"

//==================================================================
XCompUI::XCompUI( XComp &bl )
    : mXComp(bl)
{
    moConfigWin = std::make_unique<ConfigWin>( mXComp );

    moDisplayConfigWin = std::make_unique<DisplayConfigWin>( mXComp.mAppBaseConfig );
}

//==================================================================
XCompUI::~XCompUI() = default;

//==================================================================
static DStr findCommonPrefix(
                const DFun<DStr (size_t)> &getStr,
                size_t n )
{
    if ( n <= 1 )
        return {};

    c_auto str0 = getStr( 0 );
    auto minLen = str0.size();
    for (size_t i=1; i < n; ++i)
        minLen = std::min( minLen, getStr( i ).size() );

    if ( minLen == 0 )
        return {};

    size_t maxMatch = 0;
    for (size_t x=0; x < minLen; ++x)
    {
        bool broke = false;
        for (size_t i=0; i < n; ++i)
        {
            if ( getStr( i )[x] != str0[x] )
            {
                broke = true;
                break;
            }
        }

        if ( broke )
            break;
        else
            maxMatch = x;
    }

    return str0.substr( 0, maxMatch );
}

//==================================================================
void XCompUI::drawImgList()
{
    auto &imsys = *mXComp.moIMSys;

    IMUI_DrawHeader( "Images", false );

    if ( imsys.mEntries.empty() )
    {
        IMUI_Text( "No images found." );
        return;
    }

    DVec<ImageEntry *>  pEntries;
    DVec<DStr>          fNames;
    DVec<DStr>          fExts;

    for (auto it = imsys.mEntries.begin(); it != imsys.mEntries.end(); ++it)
        if ( it->second.moStdImage
#ifdef ENABLE_OPENEXR
                || it->second.moEXRImage
#endif
                )
        {
            pEntries.push_back( &it->second );

            c_auto fsPath = std::filesystem::path( it->second.mImagePathFName );

            fNames.push_back(
                    StrFromU8Str( fsPath.filename().replace_extension().u8string() ) );

            auto ext = StrFromU8Str( fsPath.extension().u8string() );
            fExts.push_back( ext.empty() ? DStr() : StrMakeUpper( ext.substr(1) ) );
        }

    //
    bool hasChangedVisibility = false;

#if 0
    c_auto commonStem = findCommonPrefix(
                            [&](auto i){ return fNames[i]; }, fNames.size() );
#else
    c_auto commonStem = DStr();
#endif

    c_auto stemIdx = commonStem.size();

    RU_IMUITableMaker tmak( "Images", 5, false );
    tmak.BeginHead();
    tmak.NewCell();
    tmak.AddText( "On" );
    tmak.NewCell();
    tmak.AddText( "Name" + (commonStem.empty() ? "" : " ("+commonStem+")") );
    tmak.NewCell();
    tmak.AddText( "Ext" );
    tmak.NewCell();
    tmak.AddText( "Size" );
    tmak.NewCell();
    tmak.AddText( "Layers" );
    tmak.EndHead();

    {
        auto makeHeadPopup = [&]( c_auto *title, auto fn )
        {
            if ( IMUI_SmallButtonEnabled( (DStr("...##") + title).c_str(), true ) )
                ImGui::OpenPopup( title );

            if ( ImGui::BeginPopup( title ) )
            {
                fn();
                ImGui::EndPopup();
            }
        };

        tmak.NewCell();
        makeHeadPopup( "AllOnPopup", [&]()
        {
            if ( ImGui::Selectable( "All ON", false ) )
            {
                hasChangedVisibility = true;
                for (auto &[k, e] : imsys.mEntries)
                    e.mIsImageEnabled = true;
            }

            if ( ImGui::Selectable( "All OFF", false ) )
            {
                hasChangedVisibility = true;
                for (auto &[k, e] : imsys.mEntries)
                    e.mIsImageEnabled = false;
            }
        });
        tmak.NewCell();
        tmak.NewCell();
        tmak.NewCell();
        tmak.NewCell();
    }

    //

    bool hasChangedLayer = false;

    c_auto savedSelPathFName = imsys.mCurSelPathFName;

    bool isInSelRange = false;

    for (size_t ii=pEntries.size(); ii > 0; --ii)
    {
        c_auto i = ii - 1;
        auto &e = *pEntries[i];

        if ( !isInSelRange && imsys.mCurSelPathFName == e.mImagePathFName )
            isInSelRange = true;

        tmak.NewCell();
        {
            c_auto col = (isInSelRange ? Display::GREEN : Display::GRAY);
            IMUI_PushStyleColor(ImGuiCol_FrameBg,        col.ScaleRGB(0.35f) );
            IMUI_PushStyleColor(ImGuiCol_FrameBgHovered, col.ScaleRGB(0.55f) );
            IMUI_PushStyleColor(ImGuiCol_CheckMark,      col  );
        }

#if 1
        if ( ImGui::Checkbox( SSPrintFS("##ImgItem%zu",i).c_str(), &e.mIsImageEnabled ) )
        {
            hasChangedVisibility = true;
        }
#else
        if ( ImGui::RadioButton( SSPrintFS("##ImgItem%zu",i).c_str(), e.mIsImageEnabled ) )
        {
            hasChangedVisibility = true;
            e.mIsImageEnabled = !e.mIsImageEnabled;
        }
#endif

        IMUI_PopStyleColor(3);

        if NOT( isInSelRange )
            IMUI_PushDisabledStyle();

        tmak.NewCell();
        if ( ImGui::Selectable(
                        fNames[i].substr(stemIdx).c_str(),
                        imsys.mCurSelPathFName == e.mImagePathFName,
                        ImGuiSelectableFlags_SpanAllColumns ) )
        {
            imsys.mCurSelPathFName = e.mImagePathFName;
        }

        tmak.NewCell();
        tmak.AddText( fExts[i] );

        size_t w = 0;
        size_t h = 0;
        size_t laysN = 0;
#ifdef ENABLE_OPENEXR
        if (c_auto &oIEXR = e.moEXRImage; oIEXR)
        {
            w = oIEXR->ie_w;
            h = oIEXR->ie_h;

            // 0 layers, if it's a fummy one
            laysN =
                (oIEXR->ie_layers.size() == 1
                    ? (oIEXR->ie_layers.front()->iel_name == ImageSystem::DUMMY_LAYER_NAME
                        ? 0 : 1)
                    : oIEXR->ie_layers.size());
        }
        else
#endif
        if ( e.moStdImage )
        {
            w = e.moStdImage->mW;
            h = e.moStdImage->mH;
            laysN = 0;
        }

        tmak.NewCell();

        if ( w || h )
            IMUI_Text( SSPrintFS("%zux%zu", w, h) );

        tmak.NewCell();

        if ( laysN )
            IMUI_Text( std::to_string( laysN ) );

        if NOT( isInSelRange )
            IMUI_PopDisabledStyle();
    }

    if ( savedSelPathFName != imsys.mCurSelPathFName || hasChangedVisibility )
        imsys.ReqRebuildComposite();
}

//==================================================================
void XCompUI::drawLayersList()
{
    auto &imsys = *mXComp.moIMSys;

    std::map<DStr,DVec<const ImageEntry*>>   pIEntsInLays;

    // start adding layers only once we find the selected image
    bool foundSelImage = false;

    for (auto it = imsys.mEntries.rbegin(); it != imsys.mEntries.rend(); ++it)
    {
        c_auto &e = it->second;

        if ( e.mImagePathFName == imsys.mCurSelPathFName )
            foundSelImage = true;

        if NOT( e.mIsImageEnabled )
            continue;

        if NOT( foundSelImage )
            continue;

#ifdef ENABLE_OPENEXR
        if (c_auto &oIEImage = e.moEXRImage; oIEImage)
        {
            for (c_auto &oLayer : oIEImage->ie_layers)
                pIEntsInLays[ oLayer->iel_name ].push_back( &it->second );
        }
        else
#endif
        {
            pIEntsInLays[ ImageSystem::DUMMY_LAYER_NAME ].push_back( &it->second );
        }
    }

    //
    if ( pIEntsInLays.empty() )
        return;

    // is current layer now invalid ?
    if (auto it = pIEntsInLays.find( imsys.mCurLayerName ); it == pIEntsInLays.end())
    {
        imsys.mCurLayerName = {};

        for (c_auto &[k, v] : pIEntsInLays)
        {
            if ( StrEndsWithI( k, "combined" )  ||
                 StrEndsWithI( k, "composite" ) ||
                 StrEndsWithI( k, "beauty" ) )
            {
                imsys.mCurLayerName = k;
                imsys.ReqRebuildComposite();
                break;
            }
        }

        if ( imsys.mCurLayerName.empty() )
        {
            imsys.mCurLayerName = pIEntsInLays.begin()->first;
            imsys.ReqRebuildComposite();
        }
    }

    if ( pIEntsInLays.size() == 1 )
        return;

    DStr commonStem;
    size_t stemIdx = DNPOS;
    {
        c_auto &refLayerName = pIEntsInLays.begin()->first;

        stemIdx = refLayerName.find_first_of( '.' );
        if ( stemIdx != DStr::npos )
        {
            commonStem = refLayerName.substr( 0, stemIdx+1 );
            for (c_auto &[k, v] : pIEntsInLays)
            {
                if NOT( StrStartsWithI( k, commonStem ) )
                {
                    commonStem = {};
                    stemIdx = DNPOS;
                    break;
                }
            }
        }
    }

    IMUI_DrawHeader( "Layers", false );

    RU_IMUITableMaker tmak( "Layers", 3, false );
    tmak.BeginHead();
    tmak.NewCell();
    tmak.AddText( "Name" + (commonStem.empty() ? "" : " ("+commonStem+")") );
    tmak.NewCell();
    tmak.AddText( "Chans" );
    tmak.NewCell();
    tmak.AddText( "Types" );
    tmak.EndHead();

    bool hasChangedLayer = false;

    for (c_auto &[k, v] : pIEntsInLays)
    {
        tmak.NewCell();
        if ( ImGui::Selectable(
                    k.c_str() + (stemIdx != DNPOS ? stemIdx+1 : 0),
                    k == imsys.mCurLayerName,
                    ImGuiSelectableFlags_SpanAllColumns ) )
        {
            hasChangedLayer = true;
            imsys.mCurLayerName = k;
        }

        if NOT( v.empty() )
        {
            DStr chNames;
            DStr chTypes;

            c_auto &e = *v.front();
#ifdef ENABLE_OPENEXR
            if ( e.moEXRImage )
            {
                if (c_auto *pLayer = e.moEXRImage->FindLayerByName( k ))
                {
                    for (c_auto &ch : pLayer->iel_chans)
                    {
                        if NOT( chNames.empty() )
                            chNames += ',';

                        if ( ch.iec_chanName.empty() )
                            continue;

                        chNames += ch.GetChanNameOnly();

                        //
                        if NOT( chTypes.empty() )
                            chTypes += ',';

                        switch ( ch.iec_dataType )
                        {
                        case IEXR_DTYPE_UINT : chTypes += 'U'; break;
                        case IEXR_DTYPE_HALF : chTypes += 'H'; break;
                        default:
                        case IEXR_DTYPE_FLOAT: chTypes += 'F'; break;
                        }
                    }
                }
            }
            else
#endif
            if ( e.moStdImage )
            {
                chNames = "R,G,B,A";
                chTypes = "8,8,8,8";
            }
            tmak.NewCell();
            IMUI_Text( chNames );
            tmak.NewCell();
            IMUI_Text( chTypes );
        }
        else
        {
            tmak.NewCell();
            tmak.NewCell();
        }
    }

    if ( hasChangedLayer )
        imsys.ReqRebuildComposite();
}

//==================================================================
void XCompUI::drawCompositeDisp()
{
    float dispW = 0;
    float dispH = 0;
    {
        const auto* window = ImGui::GetCurrentWindowRead();

        c_auto rc = window->InnerClipRect;
        c_auto pos = ImGui::GetCursorScreenPos();

        c_auto x = pos[0];
        c_auto y = pos[1];
        dispW = rc.Max[0] - x;
        dispH = rc.Max[1] - y;
    }

    c_auto &imsys = *mXComp.moIMSys;

    if NOT( imsys.moComposite )
        return;

    //
    float useDispW = 0;
    float useDispH = 0;

    c_auto &img = *imsys.moComposite;

    if ( mXComp.mConf.cfg_dispAutoFit )
    {
        c_auto sW = (float)dispW / img.mW;
        c_auto sH = (float)dispH / img.mH;

        c_auto imgWHR = (float)img.mW / img.mH;

        if ( sH > sW )
        {
            useDispW = dispW;
            useDispH = dispW / imgWHR;
        }
        else
        {
            useDispW = dispH * imgWHR;
            useDispH = dispH;
        }
    }
    else
    {
        useDispW = (float)img.mW;
        useDispH = (float)img.mH;
    }

#ifdef ENABLE_IMGUITEXINSPECT
    ImGuiTexInspect::SetZoomRate( 2.0f );

    ImGuiTexInspect::SetPanButton(
        mXComp.mConf.cfg_ctrlPanButton == "left"
            ? ImGuiMouseButton_Left
            : ImGuiMouseButton_Right );

    //if ( mXComp.mConf.cfg_dispAutoFit )
    //{
    //    ImGui::Image( (void *)(ptrdiff_t)img.GetTextureID(), { useDispW, useDispH } );
    //}
    //else
    {
        ImGuiTexInspect::BeginInspectorPanel(
                "Inspector",
                (void *)(ptrdiff_t)img.GetTextureID(),
                { (float)img.mW, (float)img.mH },
                ImGuiTexInspect::InspectorFlags_NoTooltip |
                ImGuiTexInspect::InspectorFlags_NoGrid |
                (mXComp.mConf.cfg_dispUseBilinear
                    ? ImGuiTexInspect::InspectorFlags_NoForceFilterNearest
                    : 0),
                ImGuiTexInspect::SizeIncludingBorder{
                    ImGui::GetContentRegionAvail() }
                );

        ImGuiTexInspect::SetInspectorZoomRange( 0.125f, 4.f );

        mCurZoom = ImGuiTexInspect::GetInspectorZoom();

        ImGuiTexInspect::EndInspectorPanel();
    }
#else
    ImGui::Image( (void *)(ptrdiff_t)img.GetTextureID(), { useDispW, useDispH } );
#endif
}

//==================================================================
void XCompUI::drawDisplayHead()
{
    if ( ImGui::Button( "Save composite" ) )
    {
        mXComp.SaveCompositeXC();
    }

    ImGui::SameLine();

    IMUI_Text( SSPrintFS( "%.1f%%",  mCurZoom * 100 ) );

    IMUI_LongSameLine();

    if ( ImGui::Button( "Config..." ) )
        moConfigWin->ActivateConfigWin( true );

    // visible message for possible UI freeze
    if ( mXComp.moIMSys->IsRebuildingComposite() )
    {
        ImGui::SameLine();
        IMUI_TextColored( Display::YELLOW, "Updating..." );
    }

#if 0
    ImGui::SameLine();

    if ( ImGui::Checkbox( "Autofit", &mXComp.mConf.cfg_dispAutoFit ) )
    {
        mXComp.reqLazySaveConfig();
    }
#endif
}

//==================================================================
void XCompUI::SetupGraphicsAppParams( GraphicsAppParams &par )
{
    par.mWinDefs.push_back({
          .wd_name = "About"
        , .wd_dir = {}
        , .wd_ratio = {}
        , .wd_startOpen = false
        , .wd_closeCross = true
        , .wd_winDrawFn = {}
        , .wd_winUpdateFn = {}
        , .wd_winUpdateCustomFn = [this](c_auto &, auto *pOpen )
        {
            IMUI_AboutDialogParams par;
            par.appName     = GTV_XCOMP_NAME;
            par.appLongName = GTV_XCOMP_LONGNAME;
            par.appVersion  = GTV_SUITE_VERSION;
            par.dispURL     = GTV_SUITE_DISPURL;
            par.fullURL     = GTV_SUITE_FULLURL;
            par.copyrightText= "Copyright by Gugen Studio Inc., Japan 2022";
            par.pOpen       = pOpen;
            par.val_Year    = DM_YEAR ;
            par.val_Month   = DM_MONTH;
            par.val_Day     = DM_DAY  ;
            par.val_Hours   = DM_HOURS;
            par.val_Mins    = DM_MINS ;
            par.showSmall   = false;
            IMUI_AboutDialog( par );
        }
    });

    //
    par.mWinDefs.push_back({
          .wd_name = "Log"
        , .wd_dir = "down"
        , .wd_ratio = 0.50f
        , .wd_startOpen = false
        , .wd_closeCross = true
        , .wd_winDrawFn = [this]() { mUILogger.DrawUIL( mXComp.mpGfxApp->GetIMUIFixFont() ); }
    });

    par.mpUILogger = &mUILogger;

    par.mWinDefs.push_back({
          .wd_name = "Assets"
        , .wd_dir = "left"
        , .wd_ratio = 0.25f
        , .wd_startOpen = true
        , .wd_closeCross = false
        , .wd_winDrawFn = [this]()
        {
            if ( mXComp.mConf.cfg_scanDir.empty() )
            {
                if ( ImGui::Button( "Config..." ) )
                    moConfigWin->ActivateConfigWin( true );

                return;
            }

            mRenderHasFocus = ImGui::IsWindowFocused( ImGuiFocusedFlags_ChildWindows );

            drawImgList();

            drawLayersList();
        }
    });

    par.mWinDefs.push_back({
          .wd_name = "Display"
        , .wd_dir = "right"
        , .wd_ratio = 0.75f
        , .wd_startOpen = true
        , .wd_closeCross = false
        , .wd_winDrawFn = [this]()
        {
            mDisplayHasFocus = ImGui::IsWindowFocused();

            drawDisplayHead();

            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0,0));
            ImGui::BeginChild( "ImageSub" );
            ImGui::PopStyleVar();

            mDisplayHasFocus |= ImGui::IsWindowFocused();
            drawCompositeDisp();

            ImGui::EndChild();
        }
    });

    //
    par.mOnWindowPosFn = [&]( int x, int y )
    {
        mXComp.mAppBaseConfig.mData.sys_win_pos_x = x;
        mXComp.mAppBaseConfig.mData.sys_win_pos_y = y;
        mAppBaseConfig_SaveTimeUS = GetEpochTimeUS() + TimeUS::ONE_SECOND() * 2;
        return false;
    };

    par.mOnWindowSizFn = [&]( int w, int h )
    {
#if __APPLE__
        if ( IMUI_GetIsRetinaDisplay() )
        {
            w /= 2;
            h /= 2;
        }
#endif
        c_auto contScale = IMUI_GetContentScale();
        mXComp.mAppBaseConfig.mData.sys_win_siz_w = (int)(w / contScale);
        mXComp.mAppBaseConfig.mData.sys_win_siz_h = (int)(h / contScale);
        mAppBaseConfig_SaveTimeUS = GetEpochTimeUS() + TimeUS::ONE_SECOND() * 2;
        return false;
    };

}

//==================================================================
void XCompUI::SetupGraphicsAppParamsDispConfigMenu( GraphicsAppParams &par )
{
    par.mMenuItems.push_back({
              .mi_menuName   = "File"
            , .mi_itemName   = "Configuration..."
            , .mi_pCheckOnOff = nullptr
            , .mi_onItemSelectFn = [this](){ moConfigWin->ActivateConfigWin(true); }
            , .mi_drawFn = [this](){ moConfigWin->DrawConfigWin(); }
    });

    par.mMenuItems.push_back({
              .mi_menuName   = "File"
            , .mi_itemName   = "Display Configuration..."
            , .mi_pCheckOnOff = nullptr
            , .mi_onItemSelectFn =
                [this](){ moDisplayConfigWin->ActivateDisplayConfigWin(true); }
            , .mi_drawFn =
                [this](){ moDisplayConfigWin->DrawDisplayConfigWin( mXComp.mpGfxApp ); }
    });

    par.mMenuItems.push_back({
              .mi_menuName   = "Help"
            , .mi_itemName   = "About..."
            , .mi_pCheckOnOff = nullptr
            , .mi_onItemSelectFn =
                [this](){ mXComp.mpGfxApp->SetUIWindowOpen( "About", true ); }
    });

#ifdef DEBUG
    par.mMenuItems.push_back({
              .mi_menuName   = "File"
            , .mi_itemName   = "Debug..."
            , .mi_pCheckOnOff = nullptr
            , .mi_onItemSelectFn = [this](){ mShowDebugWin = true; }
            , .mi_drawFn = [this]()
            {
                if NOT( mShowDebugWin )
                    return;

                if NOT( ImGui::Begin( "Debug", &mShowDebugWin,
                                  ImGuiWindowFlags_NoCollapse
                                | ImGuiWindowFlags_NoDocking ) )
                {
                    ImGui::End();
                    return;
                }

                if ( ImGui::Button( "Throw Runtime Exception" ) )
                    DEX_RUNTIME_ERROR( "Test exception !" );

                if ( ImGui::Button( "Generate Sig Fault" ) )
                    *((uint8_t *)0) = 0xff;

                if ( ImGui::Button( "exit( 0)" ) ) exit( 0);
                if ( ImGui::Button( "exit( 1)" ) ) exit( 1);
                if ( ImGui::Button( "exit(-1)" ) ) exit(-1);
                if ( ImGui::Button( "abort()"  ) ) abort();

                ImGui::NewLine();

                ImGui::End();
            }
    });
#endif

}

//==================================================================
void XCompUI::OnAnimateXCUI()
{
    if ( mAppBaseConfig_SaveTimeUS && GetEpochTimeUS() >= mAppBaseConfig_SaveTimeUS )
    {
        mAppBaseConfig_SaveTimeUS = {};
        mXComp.mAppBaseConfig.SaveAppBaseConfig();
    }

    if (c_auto optConfig = moConfigWin->GetChangedConfig() )
    {
        mXComp.mConf.CopyConfigVals( *optConfig );

        mXComp.reqLazySaveConfig();

        mXComp.moIMSys->mUseBilinear        = mXComp.mConf.cfg_dispUseBilinear  ;
        mXComp.moIMSys->mCCorRGBOnly        = mXComp.mConf.cfg_ccorRGBOnly      ;
        mXComp.moIMSys->mCCorSRGB           = mXComp.mConf.cfg_ccorSRGB         ;
        mXComp.moIMSys->mCCorXform          = mXComp.mConf.cfg_ccorXform        ;
        mXComp.moIMSys->mCCorOCIOCfgFName   = mXComp.mConf.cfg_ccorOCIOCfgFName ;
        mXComp.moIMSys->ReqRebuildComposite();
    }
}

