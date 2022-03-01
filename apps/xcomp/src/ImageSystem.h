//==================================================================
/// ImageSystem.h
///
/// Created by Davide Pasca - 2022/01/18
/// See the file "license.txt" that comes with this project for
/// copyright info.
//==================================================================

#ifndef IMAGESYSTEM_H
#define IMAGESYSTEM_H

#include <map>
#include "Image.h"
#include "Image_EXR.h"

//==================================================================
struct ImageEntry
{
    friend class ImageSystem;

    DStr            mImagePathFName;
    bool            mIsImageEnabled { true };
    DStr            mCurLayerForImage;
    uptr<image>     moStdImage;
#ifdef ENABLE_OPENEXR
    uptr<ImageEXR>  moEXRImage;
#endif
private:
    uptr<image>     moStdImageScaled;
public:
    ImageEntry() {}
    ImageEntry( const DStr &pathFName );

private:
    void loadStdImage();
    void loadEXRImage();
};

//==================================================================
class ImageSystem
{
public:
    inline static DStr          DUMMY_LAYER_NAME { "__default__" };
    std::map<DStr,ImageEntry>   mEntries;
    uptr<image>                 moComposite;
    DStr                        mCurSelPathFName;
    bool                        mUseBilinear = true;
    bool                        mCCorRGBOnly = true;
    bool                        mCCorSRGB = true;
    DStr                        mCCorXform   { "none" };

    DStr                        mCurLayerName;

    bool                        mHasRebuildReq = false;

private:
    std::unordered_set<DStr>    mNotifiedBadPaths;

public:
    void OnNewScanDir( const DStr &path, const DStr &selPathFName );

    void SaveComposite( const DStr &path ) const;
    bool IncCurSel( int step );
    void SetFirstCurSel();
    void SetLastCurSel();

    void ReqRebuildComposite();

    void AnimateIMS();

    bool IsRebuildingComposite() const;

private:
    void makeDummyComposite();
    void rebuildComposite();
    void makeComposite( DVec<ImageEntry *> pEntries, size_t n );
};


#endif

