/*
 * Map.cpp
 *
 *  Created on: 14.08.2019
 *      Author: user
 */

#include "Map.h"
#include "S60MapsAppView.h"
#include <eikenv.h>
#include <gdi.h>
#include <imageconversion.h>
#include <e32math.h>
#include <bitstd.h>
#include "Logger.h"
#include "S60Maps.pan"
#include "S60MapsAppUi.h"
#include "S60MapsApplication.h"
#include <bautils.h>
#include "Defs.h"

CMapLayerBase::CMapLayerBase(/*const*/ CS60MapsAppView* aMapView) :
		iMapView(aMapView)
	{
	}


#ifdef DEBUG_SHOW_ADDITIONAL_INFO

// CMapLayerDebugInfo

CMapLayerDebugInfo::CMapLayerDebugInfo(/*const*/ CS60MapsAppView* aMapView) :
	CMapLayerBase(aMapView),
	iRedrawingsCount(0)
	{
	}

void CMapLayerDebugInfo::Draw(CWindowGc &aGc)
	{
	TRAP_IGNORE(DrawInfoL(aGc));
	};

void CMapLayerDebugInfo::DrawInfoL(CWindowGc &aGc)
	{
	_LIT(KRedrawingsText, "Redrawings: %d");
	_LIT(KLatText, "Lat: %f");
	_LIT(KLonText, "Lon: %f");
	_LIT(KZoomText, "Zoom: %d");
	
	iRedrawingsCount++;
	TCoordinate center = iMapView->GetCenterCoordinate();
	
	// Prepare strings
	/*TBuf<100> buff;
		_LIT(KFmt, "Redrawings: %d\rLat: %f\rLon: %f\rZoom: %d");
		buff.Format(KFmt, iRedrawingsCount, center.Latitude(), center.Longitude(), (TInt) iMapView->GetZoom());*/
	
	CDesCArrayFlat* strings = new (ELeave) CDesCArrayFlat(4);
	CleanupStack::PushL(strings);
	
	TBuf<32> buff;
	buff.Format(KRedrawingsText, iRedrawingsCount);
	strings->AppendL(buff);
	buff.Format(KLatText, center.Latitude());
	strings->AppendL(buff);
	buff.Format(KLonText, center.Longitude());
	strings->AppendL(buff);
	buff.Format(KZoomText, (TInt) iMapView->GetZoom());
	strings->AppendL(buff);
	
	// Draw
	aGc.Reset();
	aGc.SetPenColor(KRgbDarkBlue);
	
	// FixMe: Fails with Panic KERN-EXEC 3 when program are going to exit
	const CFont* font = CEikonEnv::Static()->AnnotationFont();
	aGc.UseFont(font);
	TRect area = iMapView->Rect();
	area.Shrink(4, 4);
	TInt baselineOffset = 0;
	for (TInt i = 0; i < strings->Count(); i++)
		{
		baselineOffset += font->AscentInPixels() + 5;
		aGc.DrawText((*strings)[i], area, baselineOffset, CGraphicsContext::ERight);
		}
	aGc.DiscardFont();
	
	CleanupStack::PopAndDestroy(strings);
	}

#endif


// MTileBitmapManagerObserver
void MTileBitmapManagerObserver::OnTileLoadingFailed(const TTile &/*aTile*/, TInt /*aErrCode*/)
	{
	// No any action by default
	}


// CTiledMapLayer

CTiledMapLayer::CTiledMapLayer(CS60MapsAppView* aMapView) :
	CMapLayerBase(aMapView)
	{
	// No implementation required
	}

CTiledMapLayer::~CTiledMapLayer()
	{
	delete iBitmapMgr;
	}

CTiledMapLayer* CTiledMapLayer::NewL(CS60MapsAppView* aMapView, TTileProvider* aTileProvider)
	{
	CTiledMapLayer* self = CTiledMapLayer::NewLC(aMapView, aTileProvider);
	CleanupStack::Pop(); // self;
	return self;
	}

CTiledMapLayer* CTiledMapLayer::NewLC(CS60MapsAppView* aMapView, TTileProvider* aTileProvider)
	{
	CTiledMapLayer* self = new (ELeave) CTiledMapLayer(aMapView);
	CleanupStack::PushL(self);
	self->ConstructL(aTileProvider);
	return self;
	}

void CTiledMapLayer::ConstructL(TTileProvider* aTileProvider)
	{
	////////////////
	//iTileProvider = new (ELeave) TOsmStandardTileProvider;
	//iTileProvider = new (ELeave) TOsmCyclesTileProvider;
	//iTileProvider = new (ELeave) TOsmTransportTileProvider;
	//iTileProvider = new (ELeave) TOsmHumanitarianTileProvider;
	//iTileProvider = new (ELeave) TOpenTopoMapTileProvider;
	////////////////////
//	TBuf<32> tileProviderID;
//	iTileProvider->ID(tileProviderID);
//	iMapView->SetZoomBounds(iTileProvider->MinZoomLevel(), iTileProvider->MaxZoomLevel());
	
//	TFileName cacheDir;
//	CS60MapsAppUi* appUi = static_cast<CS60MapsAppUi*>(CCoeEnv::Static()->AppUi());
//	CS60MapsApplication* app = static_cast<CS60MapsApplication*>(appUi->Application());
//	app->CacheDir(cacheDir);
//	cacheDir.Append(tileProviderID);
//	cacheDir.Append(KPathDelimiter);
//	
//	RFs fs = iMapView->ControlEnv()->FsSession();
//	
//	// Create cache dir (if not exists)
//	TInt r = fs.MkDirAll(cacheDir);
//	if (r != KErrAlreadyExists)
//		User::LeaveIfError(r);
	
	SetTileProviderL(aTileProvider);
	
	//RFs fs = iMapView->ControlEnv()->FsSession();
	//iBitmapMgr = CTileBitmapManager::NewL(this, fs, iTileProvider, cacheDir);
	}

void CTiledMapLayer::Draw(CWindowGc &aGc)
	{
	LOG(_L8("Begin layer drawing"));
	
	RArray<TTile> tiles(10);
	VisibleTiles(tiles);
	for (TInt idx = 0; idx < tiles.Count(); idx++)
		{
		CFbsBitmap* bitmap;
		TInt err = iBitmapMgr->GetTileBitmap(tiles[idx], bitmap);
		switch (err)
			{
			case KErrNone:
				{
				DrawTile(aGc, tiles[idx], bitmap);
				break;
				}
				
			case KErrNotFound:
				{
				iBitmapMgr->AddToLoading(tiles[idx]);
				TInt err = iBitmapMgr->GetTileBitmap(tiles[idx], bitmap);
				if (KErrNone == err)
					DrawTile(aGc, tiles[idx], bitmap);
				break;
				}
				
			/*default:
				break;*/
			}
		
		}
	
	tiles.Close();
	LOG(_L8("End layer drawing"));
	}

void CTiledMapLayer::VisibleTiles(RArray<TTile> &aTiles)
	{
	TTile topLeftTile, bottomRightTile;
	iMapView->Bounds(topLeftTile, bottomRightTile);
	TUint x, y;
	for (y = topLeftTile.iY; y <= bottomRightTile.iY; y++)
		{
		for (x = topLeftTile.iX; x <= bottomRightTile.iX; x++)
			{
			TTile tile;
			tile.iX = x;
			tile.iY = y;
			tile.iZ = iMapView->GetZoom();
			aTiles.Append/*L*/(tile); // ToDo: Check error code
			}
		}
	aTiles.Compress();
	}

void CTiledMapLayer::DrawTile(CWindowGc &aGc, const TTile &aTile, const CFbsBitmap *aBitmap)
	{
	TCoordinate coord = MapMath::TileToGeoCoords(aTile, iMapView->GetZoom());
	TPoint point = iMapView->GeoCoordsToScreenCoords(coord);
	TRect destRect;
	destRect.iTl = point;
	destRect.SetSize(TSize(KTileSize, KTileSize));
	TRect screenRect = iMapView->Rect();
	if (!screenRect.Intersects(destRect)) // Check if tile is visible
		return;
	
	destRect.Intersection(screenRect);
	
	TRect srcRect = destRect;
	srcRect.Move(-point);

	
	aGc.DrawBitmap(destRect, aBitmap, srcRect);
	}

void CTiledMapLayer::OnTileLoaded(const TTile &/*aTile*/, const CFbsBitmap */*aBitmap*/)
	{
	//iMapView->DrawDeferred();
	iMapView->DrawNow();
	}

void CTiledMapLayer::SetTileProviderL(TTileProvider* aTileProvider)
	{
	iTileProvider = aTileProvider;
	
	//iMapView->SetZoomBounds(iTileProvider->MinZoomLevel(), iTileProvider->MaxZoomLevel());
	
	TFileName cacheDir;
	CS60MapsAppUi* appUi = static_cast<CS60MapsAppUi*>(CCoeEnv::Static()->AppUi());
	CS60MapsApplication* app = static_cast<CS60MapsApplication*>(appUi->Application());
	app->CacheDir(cacheDir);
	cacheDir.Append(iTileProvider->iId);
	cacheDir.Append(KPathDelimiter);
	
	RFs fs = iMapView->ControlEnv()->FsSession();
	
	// Create cache dir (if not exists)
	TInt r = fs.MkDirAll(cacheDir);
	if (r != KErrAlreadyExists)
		User::LeaveIfError(r);
	
	if (iBitmapMgr == NULL)
		// Create bitmap manager if not exist yet
		iBitmapMgr = CTileBitmapManager::NewL(this, fs, iTileProvider, cacheDir);
	else
		// Set new tile provider and cache dir for bitmap manager
		iBitmapMgr->ChangeTileProvider(iTileProvider, cacheDir);
	
	//iMapView->DrawNow();
	}


// CUserPositionLayer

CUserPositionLayer::CUserPositionLayer(/*const*/ CS60MapsAppView* aMapView) :
		CMapLayerBase(aMapView)
	{
	
	}

void CUserPositionLayer::Draw(CWindowGc &aGc)
	{
	TCoordinateEx pos;
	TInt r = iMapView->UserPosition(pos);
	if (r == KErrNone && iMapView->CheckCoordVisibility(pos))
		{
		TPoint screenPoint = iMapView->GeoCoordsToScreenCoords(pos);
		
		if (!Math::IsNaN(pos.HorAccuracy()) && pos.HorAccuracy() > 0)
			{
			// Draw accuracy circle
			TSize circleSize;
			MapMath::MetersToPixels(pos.Latitude(), iMapView->GetZoom(), pos.HorAccuracy(),
					circleSize.iWidth, circleSize.iHeight);
			circleSize.iHeight = circleSize.iWidth; // Real height may a little differ
						// due to projection limitations, but I ignore this
			DrawAccuracyCircle(aGc, screenPoint, circleSize);
			}
		
		// ToDo: Do not draw direction mark when speed is too low (about < 3 kph)
		if (!Math::IsNaN(pos.Course()))
			{ // Draw direction mark
			TRAP_IGNORE(DrawDirectionMarkL(aGc, screenPoint, pos.Course()));
			}
		else
			{
			// If there is no course information available, draw circle mark
			DrawRoundMark(aGc, screenPoint);
			}
		}
	}

void CUserPositionLayer::DrawAccuracyCircle(CWindowGc &aGc, const TPoint &aScreenPos, TSize aSize)
	{
	const TInt KAlpha = 0x50;
	TRgb fillColor   = 0xAD8B58;
	TRgb strokeColor = 0x4F3612;
	fillColor.SetAlpha(KAlpha);
	strokeColor.SetAlpha(KAlpha);
	
	aGc.SetBrushStyle(CGraphicsContext::ESolidBrush);
	aGc.SetBrushColor(fillColor);
	aGc.SetPenStyle(CGraphicsContext::ESolidPen);
	aGc.SetPenColor(strokeColor);
	aGc.SetPenSize(TSize(1, 1));

	TRect rect(TSize(0, 0));
	rect.Move(aScreenPos);
	//rect.Grow(aSize);
	rect.Grow(aSize.iWidth / 2, aSize.iHeight / 2);
	aGc.DrawEllipse(rect);
	}

void CUserPositionLayer::DrawDirectionMarkL(CWindowGc &aGc, const TPoint &aScreenPos, TReal aRotation)
	{
	// Points
	CArrayFix<TPoint>* points = new CArrayFixFlat<TPoint>(3);
	CleanupStack::PushL(points);
	
	points->AppendL(TPoint(-6, -5));
	//points->AppendL(TPoint(0, 0));
	points->AppendL(TPoint(6, -5));
	points->AppendL(TPoint(0, 13));
	
	// Rotation
	//TReal rad = aRotation * KDegToRad; // Rotation in radians
	TReal rad = (aRotation + 180.0) * KDegToRad; // Rotation in radians
	for (TInt i = 0; i < points->Count(); i++)
		{
		TReal x = points->At(i).iX;
		TReal y = points->At(i).iY;
		TReal c, s;
		TInt ret;
		ret = Math::Cos(c, rad);
		User::LeaveIfError(ret);
		ret = Math::Sin(s, rad);
		User::LeaveIfError(ret);
		TReal newX = x * c - y * s;
		TReal newY = x * s + y * c;
		ret = Math::Round(newX, newX, 0);
		User::LeaveIfError(ret);
		ret = Math::Round(newY, newY, 0);
		User::LeaveIfError(ret);
		points->At(i).iX = newX;
		points->At(i).iY = newY;
		}
	
	// Add drawing offset
	for (TInt i = 0; i < points->Count(); i++)
		points->At(i) += aScreenPos;
	
	// Drawing
	aGc.SetBrushStyle(CGraphicsContext::ESolidBrush);
	aGc.SetBrushColor(KRgbRed);
	aGc.SetPenStyle(CGraphicsContext::ESolidPen);
	aGc.SetPenSize(TSize(1, 1));
	aGc.SetPenColor(KRgbBlack);
	//aGc.DrawPolyLine(points);
	aGc.DrawPolygon(points);
	
	// Cleaning
	CleanupStack::PopAndDestroy(points);
	}

void CUserPositionLayer::DrawRoundMark(CWindowGc &aGc, const TPoint &aScreenPos)
	{
	aGc.SetBrushStyle(CGraphicsContext::ESolidBrush);
	aGc.SetBrushColor(KRgbRed);
	aGc.SetPenStyle(CGraphicsContext::ESolidPen);
	aGc.SetPenSize(TSize(1, 1));
	aGc.SetPenColor(KRgbBlack);
	
	TSize size(10, 10);
	TRect rect(aScreenPos, size);
	rect.Move(-size.iWidth / 2, -size.iHeight / 2); // Correct offset from figure center
	aGc.DrawEllipse(rect);
	}

// CImageReader

CImageReader::CImageReader(MImageReaderObserver* aObserver, CFbsBitmap* aBitmap) :
	CActive(EPriorityStandard),
	iObserver(aObserver),
	iBitmap(aBitmap)
	{
	}

CImageReader* CImageReader::NewLC(MImageReaderObserver* aObserver,
		CFbsBitmap* aBitmap, const TDesC &aFilePath, RFs &aFs)
	{
	CImageReader* self = new (ELeave) CImageReader(aObserver, aBitmap);
	CleanupStack::PushL(self);
	self->ConstructL(aFilePath, aFs);
	return self;
	}

CImageReader* CImageReader::NewL(MImageReaderObserver* aObserver,
		CFbsBitmap* aBitmap, const TDesC &aFilePath, RFs &aFs)
	{
	CImageReader* self = CImageReader::NewLC(aObserver, aBitmap, aFilePath, aFs);
	CleanupStack::Pop();
	return self;
	}

void CImageReader::ConstructL(const TDesC &aFilePath, RFs &aFs)
	{
	//iFilePath = aFilePath.AllocL();
	iDecoder = CImageDecoder::FileNewL(aFs, aFilePath);	
	CActiveScheduler::Add(this);
	}

CImageReader::~CImageReader()
	{
	Cancel();
	delete iDecoder;
	}

void CImageReader::DoCancel()
	{
	iDecoder->Cancel();
	}

void CImageReader::StartRead()
	{
	Cancel();
	iDecoder->Convert(&iStatus, *iBitmap, 0);
	SetActive();
	}

void CImageReader::RunL()
	{
	switch (iStatus.Int())
		{
		case KErrNone:
			iObserver->OnImageReaded();
			break;
			
		default:
			iObserver->OnImageReadingFailed(iStatus.Int());
			break;
		}
	}

//TInt CImageReader::RunError(TInt aError)
//	{
//	return aError;
//	}


// MImageReaderObserver

void MImageReaderObserver::OnImageReadingFailed(TInt /*aErr*/)
	{
	// No any action by default
	}


#ifdef DEBUG_SHOW_TILE_BORDER_AND_XYZ

// CTileBorderAndNumbersLayer

CTileBorderAndXYZLayer::CTileBorderAndXYZLayer(CS60MapsAppView* aMapView) :
		CMapLayerBase(aMapView)
	{
	}
	
void CTileBorderAndXYZLayer::Draw(CWindowGc &aGc)
	{
	RArray<TTile> tiles(10);
	VisibleTiles(tiles);
	for (TInt idx = 0; idx < tiles.Count(); idx++)
		DrawTile(aGc, tiles[idx]);
	
	tiles.Close();
	}

void CTileBorderAndXYZLayer::VisibleTiles(RArray<TTile> &aTiles)
	{
	TTile topLeftTile, bottomRightTile;
	iMapView->Bounds(topLeftTile, bottomRightTile);
	TUint x, y;
	for (y = topLeftTile.iY; y <= bottomRightTile.iY; y++)
		{
		for (x = topLeftTile.iX; x <= bottomRightTile.iX; x++)
			{
			TTile tile;
			tile.iX = x;
			tile.iY = y;
			tile.iZ = iMapView->GetZoom();
			aTiles.Append/*L*/(tile); // ToDo: Check error code
			}
		}
	aTiles.Compress();
	}

void CTileBorderAndXYZLayer::DrawTile(CWindowGc &aGc, const TTile &aTile)
	{
	// Calculate tile position
	TCoordinate coord = MapMath::TileToGeoCoords(aTile, iMapView->GetZoom());
	TPoint point = iMapView->GeoCoordsToScreenCoords(coord);
	TRect rect(TSize(KTileSize, KTileSize));
	rect.Move(point);
	
	// Draw tile boundary
	aGc.SetPenColor(KRgbDarkBlue);
	aGc.SetPenStyle(CGraphicsContext::EDottedPen);
	aGc.SetPenSize(TSize(1, 1));
	aGc.SetBrushStyle(CGraphicsContext::ENullBrush);
	aGc.DrawRect(rect);
	
	// Draw tile numbers
	_LIT(KTextFormat, "x=%d y=%d z=%d");
	TBuf<30> buff;
	buff.Format(KTextFormat, aTile.iX, aTile.iY, aTile.iZ);
	const CFont* font = CEikonEnv::Static()->SymbolFont();
	aGc.UseFont(font);
	TInt baseline = rect.Height() / 2 + font->AscentInPixels() / 2;
	aGc.DrawText(buff, rect, baseline, CGraphicsContext::ECenter);
	aGc.DiscardFont();
	}
#endif

// CTileBitmapManager

CTileBitmapManager::CTileBitmapManager(MTileBitmapManagerObserver *aObserver,
		RFs aFs, TTileProvider* aTileProvider, TInt aLimit) :
		CActive(EPriorityStandard),
		iObserver(aObserver),
		iLimit(aLimit),
		iState(/*TProcessingState::*/EIdle),
		iFs(aFs),
		iTileProvider(aTileProvider)
	{
	// No implementation required
	}

CTileBitmapManager::~CTileBitmapManager()
	{
	delete iFileMapper;
	delete iImgDecoder;
	iItemsLoadingQueue.Close();
	iItems.ResetAndDestroy();
	iItems.Close();
	delete iHTTPClient;
	}

CTileBitmapManager* CTileBitmapManager::NewLC(MTileBitmapManagerObserver *aObserver,
		RFs aFs, TTileProvider* aTileProvider, const TDesC &aCacheDir, TInt aLimit)
	{
	CTileBitmapManager* self = new (ELeave) CTileBitmapManager(aObserver, aFs, aTileProvider, aLimit);
	CleanupStack::PushL(self);
	self->ConstructL(aCacheDir);
	return self;
	}

CTileBitmapManager* CTileBitmapManager::NewL(MTileBitmapManagerObserver *aObserver,
		RFs aFs, TTileProvider* aTileProvider, const TDesC &aCacheDir, TInt aLimit)
	{
	CTileBitmapManager* self = CTileBitmapManager::NewLC(aObserver, aFs, aTileProvider, aCacheDir, aLimit);
	CleanupStack::Pop(); // self;
	return self;
	}

void CTileBitmapManager::ConstructL(const TDesC &aCacheDir)
	{
#ifdef __WINSCW__
	// Add some delay for network services have been started on the emulator,
	// otherwise CEcmtServer: 3 panic will be raised.
	User::After(10 * KSecond);
#endif
	iHTTPClient = CHTTPClient::NewL(this);
	
	TBuf8<32> userAgent;
	userAgent.Copy(_L8("S60Maps")); // ToDo: Move to constant
	userAgent.Append(' ');
	userAgent.Append('v');
	userAgent.Append(KProgramVersion.Name());
#ifdef _DEBUG
	_LIT8(KDebugStr, "DEV");
	userAgent.Append(' ');
	userAgent.Append(KDebugStr);
#endif
	iHTTPClient->SetUserAgentL(userAgent); 
	
	iItems = RPointerArray<CTileBitmapManagerItem>(iLimit);
	iItemsLoadingQueue = RArray<TTile>(20); // ToDo: Move 20 to constant
	
	iImgDecoder = CBufferedImageDecoder::NewL(iFs);
	
	iFileMapper = CFileTreeMapper::NewL(aCacheDir, 2, 1, ETrue);
	
	CActiveScheduler::Add(this);
	}

TInt CTileBitmapManager::GetTileBitmap(const TTile &aTile, CFbsBitmap* &aBitmap)
	{
	CTileBitmapManagerItem* item = Find(aTile);
	LOG(_L8("tile=%S, item=%x"), &aTile.AsDes8(), item);
	
	if (item == NULL)
		return KErrNotFound;
	LOG(_L8("tile=%S, isready=%d, bitmap=%x"), &aTile.AsDes8(), item->IsReady(), item->Bitmap());
	
	if (!item->IsReady())
		return KErrNotReady;
	
	aBitmap = item->Bitmap();
	return KErrNone;
	}

void CTileBitmapManager::AddToLoading(const TTile &aTile)
	{
	CTileBitmapManagerItem* item = Find(aTile);
	
	if (item == NULL)
		Append(aTile);
	}

/*TInt*/ void CTileBitmapManager::Append/*L*/(const TTile &aTile)
	{
	if (iItems.Count() >= iLimit)
		{
		// Delete oldest item
		LOG(_L8("Delete old bitmap of %S from cache"), &iItems[0]->Tile().AsDes8());
		delete iItems[0];
		iItems.Remove(0);
		}
	
	// Add new one
	CTileBitmapManagerItem* item = CTileBitmapManagerItem::NewL(aTile/*, iObserver*/);
	iItems.Append(item);
	
	if (iState == EIdle)
		{
		// Try to find on disk first
		if (IsTileFileExists(aTile))
			{
			item->CreateBitmapIfNotExistL();
			LoadBitmapL(aTile, item->Bitmap());
			item->SetReady();
			}
		else
			// Start download now
			StartDownloadTileL(aTile);
		}
	else
		{
		// Add to loading queue
		// ToDo: Check array is not full
		iItemsLoadingQueue.Append(aTile);
		LOG(_L8("Tile %S appended to download queue"), &aTile.AsDes8());
		LOG(_L8("Total %d tiles in download queue"), iItemsLoadingQueue.Count());
		}
	LOG(_L8("Now %d items in bitmap cache"), iItems.Count());
	}

CTileBitmapManagerItem* CTileBitmapManager::Find(const TTile &aTile) const
	{
	if (!iItems.Count())
		return NULL;
	
	for (TInt idx = iItems.Count() - 1; idx >= 0; idx--) // Needed items more often
											// located at the end of array (newest)
		{
		if (iItems[idx]->Tile() == aTile)
			return iItems[idx];
		}
	
	return NULL;
	}

void CTileBitmapManager::StartDownloadTileL(const TTile &aTile)
	{
	if (iIsOfflineMode)
		return;
	
	if (iState != /*TProcessingState::*/EIdle)
		return;
	
	iState = /*TProcessingState::*/EDownloading;
	iLoadingTile = aTile;
	
	RBuf8 tileUrl;
	const TInt KReserveLength = 30; // For variables substitution
	tileUrl.CreateL(iTileProvider->iTileUrlTemplate.Length() + KReserveLength);
	tileUrl.CleanupClosePushL();
	iTileProvider->TileUrl(tileUrl, aTile);
	iHTTPClient->GetL(tileUrl);
	LOG(_L8("Started download tile %S from url %S"), &aTile.AsDes8(), &tileUrl);
	CleanupStack::PopAndDestroy(&tileUrl);
	}

void CTileBitmapManager::DoCancel()
	{
	DEBUG(_L("Cancel"));
	iImgDecoder->Cancel();
	}

void CTileBitmapManager::RunL()
	{
	LOG(_L8("CTileBitmapManager::RunL"));
	if (iStatus.Int() == KErrNone)
		{
		/*CFbsBitmap* bitmap;
		TInt r = GetTileBitmap(iLoadingTile, bitmap);
		__ASSERT_DEBUG(r == KErrNone, User::Leave(KErrNotFound));
		CTileBitmapManagerItem* item = Find(iLoadingTile);
		__ASSERT_DEBUG(item != NULL, User::Leave(KErrNotFound));*/
		CTileBitmapManagerItem* item = Find(iLoadingTile);
		__ASSERT_DEBUG(item != NULL, Panic(ES60MapsTileBitmapManagerItemNotFoundPanic));
		__ASSERT_DEBUG(item->Bitmap() != NULL, Panic(ES60MapsTileBitmapIsNullPanic));
		
		item->SetReady();
		
		LOG(_L8("Tile %S downloaded and decoded"), &iLoadingTile.AsDes8());
		iObserver->OnTileLoaded(iLoadingTile, item->Bitmap());
		
		SaveBitmapL(iLoadingTile, item->Bitmap());
		}
	else
		{
		LOG(_L8("Image decoding error: %d"), iStatus.Int());
		iObserver->OnTileLoadingFailed(iLoadingTile, iStatus.Int());
		}
	
	
	iImgDecoder->Reset();
	iState = /*TProcessingState::*/EIdle;
	
	// Start download next tile in queue
	if (iItemsLoadingQueue.Count())
		{
		TTile tile = iItemsLoadingQueue[0]; 
		iItemsLoadingQueue.Remove(0);
		
		StartDownloadTileL(tile);
		}
	}

/*TInt CTileBitmapManager::RunError(TInt aError)
	{
	return aError;
	}*/

void CTileBitmapManager::OnHTTPResponseDataChunkRecieved(
		const RHTTPTransaction aTransaction, const TDesC8 &aDataChunk,
		TInt anOverallDataSize, TBool anIsLastChunk)
	{
	LOG(_L8("HTTP chunk recieved"));
	
	// Checking that mime-type is PNG
	// (If any error (for example: 404 Not Found) chunk may contains
	// HTML/text data instead correct PNG image. In this case, 
	// we need to skip any processing.)
	RStringPool strP = aTransaction.Session().StringPool();
	RHTTPHeaders respHeaders = aTransaction.Response().GetHeaderCollection();
	RStringF fieldName = strP.StringF(HTTP::EContentType, RHTTPSession::GetTable());
	THTTPHdrVal fieldVal;
	TInt r = respHeaders.GetField(fieldName, 0, fieldVal);
	__ASSERT_DEBUG(r == KErrNone, Panic(ES60MapsNoRequiredHeaderInResponse)); // Unlikely if response don`t contains Content-Type header
	if (r != KErrNone)
		return;
	
	_LIT8(KPNGMimeType, "image/png");
	RStringF pngMimeType = strP.OpenFStringL(KPNGMimeType);
	if (fieldVal.StrF() != pngMimeType)
		{
		pngMimeType.Close();
		return; // Skip other types exept PNG
		}
	pngMimeType.Close();
	
	
	// Append data to decoder`s buffer
	iImgDecoder->AppendDataL(aDataChunk);
	
	if (!iImgDecoder->ValidDecoder())
		iImgDecoder->ContinueOpenL();
	
	if (!iImgDecoder->ValidDecoder())
		return; // Next function will leave if decoder not created
	
	if (!iImgDecoder->IsImageHeaderProcessingComplete())
		iImgDecoder->ContinueProcessingHeaderL();
	
	//iImgDecoder->AppendDataL(aDataChunk);
	//iImgDecoder->ContinueConvert();
	}

void CTileBitmapManager::OnHTTPResponse(const RHTTPTransaction aTransaction)
	{
	LOG(_L8("HTTP response success"));
	
	iState = /*TProcessingState::*/EDecoding;
	
	// Start convert PNG to CFbsBitmap
	//CFbsBitmap* bitmap;
//	TInt r = GetTileBitmap(iLoadingTile, bitmap);
//	__ASSERT_DEBUG(r != KErrNotFound, User::Leave(KErrNotFound));
	CTileBitmapManagerItem* item = Find(iLoadingTile);
	__ASSERT_DEBUG(item != NULL, Panic(ES60MapsTileBitmapManagerItemNotFoundPanic));
	item->CreateBitmapIfNotExistL();
	__ASSERT_DEBUG(item->Bitmap() != NULL, Panic(ES60MapsTileBitmapIsNullPanic));
	
	//iImgDecoder->ContinueOpenL();
	LOG(_L8("Tile %S succesfully downloaded, starting decode"), &iLoadingTile.AsDes8());
	iImgDecoder->Convert(&this->iStatus, /**bitmap*/ *item->Bitmap(), 0);
	//iImgDecoder->ContinueConvert(&this->iStatus);
	SetActive();
	}

void CTileBitmapManager::OnHTTPError(TInt aError,
		const RHTTPTransaction aTransaction)
	{
	//LOG(_L8("HTTP error: %d"), aError);
	LOG(_L8("Failed to download tile %S, error: %d"), &iLoadingTile.AsDes8(), aError);
	iObserver->OnTileLoadingFailed(iLoadingTile, aError);
	
	iImgDecoder->Reset();
	iState = /*TProcessingState::*/EIdle;
	
	
	if (aError == KErrCancel)
		{
		// If access point not provoded switch to offline mode
		
		// FixMe: Access point choosing dialog appears several times in a row
		// (in my case: 2 in emulator, 5-6 on the phone) and only after that
		// we can catch cancel in this callback
		iIsOfflineMode = ETrue;
		LOG(_L8("Switched to Offline Mode"));
		iItemsLoadingQueue.Reset(); // Clear queue of loading tiles
		}
	else
		{	
		// Start download next tile in queue
		if (iItemsLoadingQueue.Count())
			{
			TTile tile = iItemsLoadingQueue[0]; 
			iItemsLoadingQueue.Remove(0);
			
			StartDownloadTileL(tile);
			}
		}
	}
void CTileBitmapManager::OnHTTPHeadersRecieved(
		const RHTTPTransaction aTransaction)
	{
	LOG(_L8("HTTP headers recieved"));
	
	iImgDecoder->Reset();
	_LIT8(KPNGMimeType, "image/png");
	iImgDecoder->OpenL(KNullDesC8, KPNGMimeType);
	}

void CTileBitmapManager::SaveBitmapL(const TTile &aTile, /*const*/ CFbsBitmap *aBitmap/*, TBool aRewrite*/)
	{
	TFileName tileFileName;
	TileFileName(aTile, tileFileName);

	// Create subdirs
	// ToDo: I think it will be better to create all subdirs once when program starts
	BaflUtils::EnsurePathExistsL(iFs, tileFileName);
	
	RFile file;
	/*if (aRewrite)
		{*/
		User::LeaveIfError(file.Replace(iFs, tileFileName, EFileWrite));
		CleanupClosePushL(file);
	/*	}
	else
		{
		TInt r = file.Create(iFs, tileFileName, EFileWrite);
		CleanupClosePushL(file);
		if (r != KErrAlreadyExists)
			User::LeaveIfError(r);
		}*/
	User::LeaveIfError(aBitmap->Save(file));	
	CleanupStack::PopAndDestroy(&file);
	LOG(_L8("Bitmap for %S sucessfully saved to file \"%S\""), &aTile.AsDes8(), &tileFileName);
	}

void CTileBitmapManager::LoadBitmapL(const TTile &aTile, CFbsBitmap *aBitmap)
	{	
	TFileName tileFileName;
	TileFileName(aTile, tileFileName);
	
	RFile file;
	User::LeaveIfError(file.Open(iFs, tileFileName, EFileRead));
	CleanupClosePushL(file);
	User::LeaveIfError(aBitmap->Load(file));	
	CleanupStack::PopAndDestroy(&file);
	LOG(_L("Bitmap for %S sucessfully loaded from file \"%S\""), &aTile.AsDes(), &tileFileName);
	}

TBool CTileBitmapManager::IsTileFileExists(const TTile &aTile) /*const*/
	{
	TFileName tileFileName;
	TileFileName(aTile, tileFileName);
	return BaflUtils::FileExists(iFs, tileFileName);
	}

void CTileBitmapManager::TileFileName(const TTile &aTile, TFileName &aFileName) const
	{
	_LIT(KUnderline, "_");
	_LIT(KMBMExtension, ".mbm");
	
	/*TFileName*/ TBuf<32> originalFileName;
	originalFileName.AppendNum(aTile.iZ);
	originalFileName.Append(KUnderline);
	originalFileName.AppendNum(aTile.iX);
	originalFileName.Append(KUnderline);
	originalFileName.AppendNum(aTile.iY);
	originalFileName.Append(KMBMExtension);
	
	iFileMapper->GetFilePath(originalFileName, aFileName);
	}

void CTileBitmapManager::ChangeTileProvider(TTileProvider* aTileProvider,
		const TDesC &aCacheDir)
	{
	// FixMe: On the program startup this method may be called twice with same tile provider 
	if (iTileProvider->iId == aTileProvider->iId)
		return; // Nothing changed

	INFO(_L("Changing of tile provider from %S to %S"), &iTileProvider->iTitle, &aTileProvider->iTitle);
	
	Cancel();	
	iItemsLoadingQueue.Reset(); // Should already be cleared by Cancel() call at previous line
	iItems.ResetAndDestroy();
	iTileProvider = aTileProvider;
	iFileMapper->SetBaseDir(aCacheDir);
	}

// CTileBitmapManagerItem

CTileBitmapManagerItem::~CTileBitmapManagerItem()
	{
	// FixMe: Bitmap pointer maybe still used outside when deleting
	delete iBitmap; // iBitmap already may be NULL
	
	if (iBitmap != NULL)
		LOG(_L8("Bitmap of %S destroyed"), &iTile.AsDes8());
	
	LOG(_L8("Bitmap manager item of %S destroyed"), &iTile.AsDes8());
	}

CTileBitmapManagerItem* CTileBitmapManagerItem::NewL(const TTile &aTile)
	{
	CTileBitmapManagerItem* self = CTileBitmapManagerItem::NewLC(aTile);
	CleanupStack::Pop(); // self;
	return self;
	}

CTileBitmapManagerItem* CTileBitmapManagerItem::NewLC(const TTile &aTile)
	{
	CTileBitmapManagerItem* self = new (ELeave) CTileBitmapManagerItem(aTile);
	CleanupStack::PushL(self);
	self->ConstructL();
	LOG(_L8("Bitmap manager item of %S created"), &self->iTile.AsDes8());
	return self;
	}

CTileBitmapManagerItem::CTileBitmapManagerItem(const TTile &aTile) :
		iTile(aTile)
	{
	// No implementation required
	}

void CTileBitmapManagerItem::ConstructL()
	{
	// Second phase construction is not used at the moment
	}

void CTileBitmapManagerItem::CreateBitmapIfNotExistL()
	{
	if (iBitmap != NULL)
		return;
	
	iBitmap = new (ELeave) CFbsBitmap();
	TSize size(KTileSize, KTileSize);
	TDisplayMode mode = EColor16M;
	User::LeaveIfError(iBitmap->Create(size, mode));
	}

// TTileProvider

TTileProvider::TTileProvider(const TDesC& anId, const TDesC& aTitle,
		const TDesC8& anUrlTemplate, TZoom aMinZoom, TZoom aMaxZoom)
	{
	iId.Copy(anId);
	iTitle.Copy(aTitle);
	iTileUrlTemplate.Copy(anUrlTemplate);
	iMinZoomLevel = aMinZoom;
	iMaxZoomLevel = aMaxZoom;
	}

void TTileProvider::TileUrl(TDes8 &aUrl, const TTile &aTile)
	{
	aUrl.Zero();
	TLex8 lex(iTileUrlTemplate);

	do
		{
		/*TInt*/ TUint32 value;
		TChar randChar;
		
		if (ParseVariable(lex, aTile, value) == KErrNone)
			{
			// Try to parse variable
			aUrl.AppendNum(value);
			}
		else if (ParseRandCharRange(lex, randChar) == KErrNone)
			{
			// Try to parse random char range
			aUrl.Append(randChar);
			}
		else
			{
			// Copy char to destination string
			aUrl.Append(lex.Get());
			}
			
		} while (!lex.Eos());
	}

TInt TTileProvider::ParseRandCharRange(TLex8 &aLex, TChar &aReturnedChar)
	{
	const TChar KStartChar = '{';
	const TChar KEndChar = '}';
	
	TLexMark8 startMark;
	aLex.Mark(startMark);
	
	if (aLex.Eos() || aLex.Peek() != KStartChar)
		{
		aLex.UnGetToMark(startMark);
		return KErrNotFound;
		}
	aLex.Inc();
	
	// Process start char
	if (aLex.Eos() || !aLex.Peek().IsAlphaDigit())
		{
		aLex.UnGetToMark(startMark);
		return KErrNotFound;
		}
	TChar ch1 = aLex.Get();
	
	if (aLex.Eos() || aLex.Peek() != '-')
		{
		aLex.UnGetToMark(startMark);
		return KErrNotFound;
		}
	aLex.Inc();
	
	// Process end char
	if (aLex.Eos() || !aLex.Peek().IsAlphaDigit())
		{
		aLex.UnGetToMark(startMark);
		return KErrNotFound;
		}
	TChar ch2 = aLex.Get();
	
	if (aLex.Eos() || aLex.Peek() != KEndChar)
		{
		aLex.UnGetToMark(startMark);
		return KErrNotFound;
		}
	aLex.Inc();
	
	// Check that start char is smaller than end char
	if (!(ch1 < ch2))
		{
		aLex.UnGetToMark(startMark);
		return KErrGeneral;
		}
	
	// Get random char from range
//	aReturnedChar = ch1 + (Math::Random() % (ch2 - ch1 + 1));
	aReturnedChar = TUint32(ch1) + (Math::Random() % (TUint32(ch2) - TUint32(ch1) + 1));
	return KErrNone;
	}

TInt TTileProvider::ParseVariable(TLex8 &aLex, const TTile aTile, /*TInt32*/ TUint32 &aReturnedVal)
	{
	const TChar KStartChar = '{';
	const TChar KEndChar = '}';
	const TChar KVariablePrefixChar = '$';
	
	TLexMark8 startMark;
	aLex.Mark(startMark);
	
	if (aLex.Eos() || aLex.Peek() != KStartChar)
		{
		aLex.UnGetToMark(startMark);
		return KErrNotFound;
		}
	aLex.Inc();
	
	if (aLex.Eos() || aLex.Peek() != KVariablePrefixChar)
		{
		aLex.UnGetToMark(startMark);
		return KErrNotFound;
		}
	aLex.Inc();
	
	if (aLex.Eos())
		{
		aLex.UnGetToMark(startMark);
		return KErrNotFound;
		}
	
	TChar variable = aLex.Get();
	/*TInt32*/ TUint32 value = KNaN;
	switch (variable)
		{
		case 'x':
			value = aTile.iX;
			break;
			
		case 'y':
			value = aTile.iY;
			break;
							
		case 'z':
			value = aTile.iZ;
			break;
			
		default:
			aLex.UnGetToMark(startMark);
			return KErrNotFound;
			break;
		}
	
	if (aLex.Eos() || aLex.Peek() != KEndChar)
		{
		aLex.UnGetToMark(startMark);
		return KErrNotFound;
		}
	aLex.Inc();
	
	aReturnedVal = value;
	
	return KErrNone;	
	}



// TCoordinateEx

TCoordinateEx::TCoordinateEx() /*:
		iLatitude(KNaN),
		iLongitude(KNaN),
		iAltitude(KNaN),
		iCourse(KNaN)*/
	{
	iLatitude  = KNaN;
	iLongitude = KNaN;
	iAltitude  = KNaN;
	iCourse    = KNaN;
	iHorAccuracy = KNaN;
	}

TCoordinateEx::TCoordinateEx(const TCoordinateEx &aCoordEx) /*:
		iLatitude(aCoordEx.Latitude()),
		iLongitude(aCoordEx.Longitude()),
		iAltitude(aCoordEx.Altitude()),
		iCourse(aCoordEx.Course())*/
	{
	iLatitude  = aCoordEx.Latitude();
	iLongitude = aCoordEx.Longitude();
	iAltitude  = aCoordEx.Altitude();
	iCourse    = aCoordEx.Course();
	iHorAccuracy = aCoordEx.HorAccuracy();
	}

TCoordinateEx::TCoordinateEx(const TCoordinate &aCoord) /*:
		iLatitude(aCoord.Latitude()),
		iLongitude(aCoord.Longitude()),
		iAltitude(aCoord.Altitude()),
		iCourse(KNaN)*/
	{
	iLatitude  = aCoord.Latitude();
	iLongitude = aCoord.Longitude();
	iAltitude  = aCoord.Altitude();
	iCourse    = KNaN;
	iHorAccuracy = KNaN;
	}
