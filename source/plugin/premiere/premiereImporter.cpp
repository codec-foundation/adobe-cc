#include <codecvt>
#include <new>

#include "async_importer.hpp"
#include "codec_registration.hpp"
#include "importer.hpp"
#include "logging.hpp"
#include "prstring.hpp"
#include "string_conversion.hpp"

#ifdef PRMAC_ENV
#include <mach-o/dyld.h> //!!! hack using _NSGetExecutablePath
#include <string>        //!!! for hack
#endif

using namespace std::chrono_literals;

//!!! if importerID can be removed rename this to AdobeSuites
AdobeImporterAPI::AdobeImporterAPI(piSuitesPtr piSuites)
{
    memFuncs = piSuites->memFuncs;
    BasicSuite = piSuites->utilFuncs->getSPBasicSuite();
    if (BasicSuite)
    {
        BasicSuite->AcquireSuite(kPrSDKPPixCreatorSuite, kPrSDKPPixCreatorSuiteVersion, (const void**)&PPixCreatorSuite);
        BasicSuite->AcquireSuite(kPrSDKPPixCacheSuite, kPrSDKPPixCacheSuiteVersion, (const void**)&PPixCacheSuite);
        BasicSuite->AcquireSuite(kPrSDKPPixSuite, kPrSDKPPixSuiteVersion, (const void**)&PPixSuite);
        BasicSuite->AcquireSuite(kPrSDKTimeSuite, kPrSDKTimeSuiteVersion, (const void**)&TimeSuite);
    }
    else
        throw std::runtime_error("no BasicSuite available");
}

AdobeImporterAPI::~AdobeImporterAPI()
{
    BasicSuite->ReleaseSuite(kPrSDKPPixCreatorSuite, kPrSDKPPixCreatorSuiteVersion);
    BasicSuite->ReleaseSuite(kPrSDKPPixCacheSuite, kPrSDKPPixCacheSuiteVersion);
    BasicSuite->ReleaseSuite(kPrSDKPPixSuite, kPrSDKPPixSuiteVersion);
    BasicSuite->ReleaseSuite(kPrSDKTimeSuite, kPrSDKTimeSuiteVersion);
}

static prMALError 
ImporterInit(
    imStdParms        *stdParms, 
    imImportInfoRec    *importInfo)
{
    csSDK_int32 premiereSig = reinterpret_cast<const csSDK_int32&>(CodecRegistry::codec()->details().premiereSig);
    importInfo->importerType = premiereSig;

    importInfo->setupOnDblClk = kPrFalse;
    importInfo->canSave = kPrFalse;

    // imDeleteFile8 is broken on MacOS when renaming a file using the Save Captured Files dialog
    // So it is not recommended to set this on MacOS yet (bug 1627325)

    importInfo->canDelete = kPrFalse;
    importInfo->dontCache = kPrFalse;		// Don't let Premiere cache these files
    importInfo->hasSetup = kPrFalse;		// Set to kPrTrue if you have a setup dialog
    importInfo->keepLoaded = kPrFalse;		// If you MUST stay loaded use, otherwise don't: play nice
    importInfo->priority = 100;
    importInfo->canTrim = kPrFalse;
    importInfo->canCalcSizes = kPrFalse;
    if (stdParms->imInterfaceVer >= IMPORTMOD_VERSION_6)
    {
        importInfo->avoidAudioConform = kPrTrue;
    }

    return imIsCacheable;
}

static prMALError 
ImporterGetPrefs8(
    imStdParms          *stdParms, 
    imFileAccessRec8    *fileInfo8, 
    imGetPrefsRec       *prefsRec)
{
    ImporterLocalRec8   *ldata;

    // Note: if canOpen is not set to kPrTrue, I'm not getting this selector. Why?
    // Answer: because this selector is associated directly with "hasSetup"

    if(prefsRec->prefsLength == 0)
    {
        // first time we are called we will have to supply prefs data.
        // reterun size of the buffer to store prefs.
        prefsRec->prefsLength = sizeof(ImporterLocalRec8);
    }
    else
    {
        ldata = (ImporterLocalRec8 *)prefsRec->prefs;
        //!!! could copy things into prefsRec->
    }
    return malNoError;
}


prMALError
ImporterOpenFile8(
    imStdParms		*stdParms,
    imFileRef		*,
    imFileOpenRec8	*fileOpenRec8)
{
    prMALError			result = malNoError;
    ImporterLocalRec8H	localRecH;
    const auto codec = *CodecRegistry::codec();

    if (fileOpenRec8->privatedata)
    {
        localRecH = (ImporterLocalRec8H)fileOpenRec8->privatedata;
    }
    else
    {
        localRecH = (ImporterLocalRec8H)stdParms->piSuites->memFuncs->newHandle(sizeof(ImporterLocalRec8));
        fileOpenRec8->privatedata = (void*)localRecH;

        // Construct it
        new (*localRecH) ImporterLocalRec8(SDKStringConvert::to_wstring(fileOpenRec8->fileinfo.filepath));
    }

    // open the file
    try {
        FileFormat fileFormat = codec.details().fileFormat;

        (*localRecH)->movieReader = createMovieReader(fileFormat, (*localRecH)->filePath);

        fileOpenRec8->fileinfo.filetype = reinterpret_cast<csSDK_int32&>(fileFormat);

        (*localRecH)->importerID = fileOpenRec8->inImporterID;

        FrameFormat format((codec.details().isHighBitDepth ? ChannelFormat_U16_32k : ChannelFormat_U8) // we're going to try to deliver something in keeping with the codec's high bit depth
            | FrameOrigin_BottomLeft
            | ChannelLayout_BGRA      // bgra
        );

        auto decoderParameters = std::make_unique<DecoderParametersBase>(
            FrameDef(FrameSize{(*localRecH)->movieReader->width(), (*localRecH)->movieReader->height()},
                     format
                    )
            );
        (*localRecH)->decoder = codec.createDecoder(std::move(decoderParameters));
        (*localRecH)->decoderJob = (*localRecH)->decoder->create();
    }
    catch (...)
    {
        result = imBadFile;
    }

    return result;
}


//-------------------------------------------------------------------
//	"Quiet" the file (it's being closed, but you maintain your Private data).  
//	
//	NOTE:	If you don't set any privateData, you will not get an imCloseFile call
//			so close it up here.

static prMALError
ImporterQuietFile(
    imStdParms			*stdParms,
    imFileRef			*,
    void				*privateData)
{
    ImporterLocalRec8H localRecH = (ImporterLocalRec8H)privateData;
    (*localRecH)->movieReader.reset(0);

    return malNoError;
}

//-------------------------------------------------------------------
//-------------------------------------------------------------------
//	Close the file.  You MUST have allocated Private data in imGetPrefs or you will not
//	receive this call.

static prMALError
ImporterCloseFile(
    imStdParms			*stdParms,
    imFileRef			*,
    void				*privateData)
{
    ImporterLocalRec8H ldataH = reinterpret_cast<ImporterLocalRec8H>(privateData);

    // Remove the privateData handle.
    // CLEANUP - Destroy the handle we created to avoid memory leaks
    if (ldataH && *ldataH) //!!!  && (*ldataH)->BasicSuite)  either it was constructed, or its null.
    {
        (*ldataH)->~ImporterLocalRec8();
        stdParms->piSuites->memFuncs->disposeHandle(reinterpret_cast<char**>(ldataH));
    }

    return malNoError;
}


static prMALError 
ImporterGetIndFormat(
    imStdParms     *stdParms, 
    csSDK_size_t    index, 
    imIndFormatRec *indFormatRec)
{
    prMALError    result        = malNoError;
    char platformXten[256]  = "MOV\0\0";

    switch(index)
    {
        //    Add a case for each filetype.
        
    case 0:
    {
        const auto codec = *CodecRegistry::codec();
        FileFormat fileFormat = codec.details().fileFormat;
        indFormatRec->filetype = reinterpret_cast<csSDK_int32&>(fileFormat);

        indFormatRec->canWriteTimecode    = kPrTrue;

        #ifdef PRWIN_ENV
        strcpy_s(indFormatRec->FormatName, sizeof (indFormatRec->FormatName), codec.details().fileFormatName.c_str());                 // The long name of the importer
        strcpy_s(indFormatRec->FormatShortName, sizeof (indFormatRec->FormatShortName), codec.details().fileFormatShortName.c_str());        // The short (menu name) of the importer
        strcpy_s(indFormatRec->PlatformExtension, sizeof (indFormatRec->PlatformExtension), platformXten);  // The 3 letter extension
        #else
        strcpy(indFormatRec->FormatName, codec.details().fileFormatName.c_str());            // The Long name of the importer
        strcpy(indFormatRec->FormatShortName, codec.details().fileFormatShortName.c_str());  // The short (menu name) of the importer
        strcpy(indFormatRec->PlatformExtension, platformXten);   // The 3 letter extension
        #endif

        break;
    }
    default:
        result = imBadFormatIndex;
    }
    return result;
}

static prMALError
ImporterGetIndPixelFormat(
    imStdParms			*stdParms,
    csSDK_size_t		idx,
    imIndPixelFormatRec	*SDKIndPixelFormatRec)
{
    prMALError	result = malNoError;

    switch (idx)
    {
    case 0:
        SDKIndPixelFormatRec->outPixelFormat = CodecRegistry::codec()->details().isHighBitDepth
            ? PrPixelFormat_BGRA_4444_16u // PrPixelFormat_BGRA_4444_32f
            : PrPixelFormat_BGRA_4444_8u;
        break;

    default:
        result = imBadFormatIndex;
        break;
    }
    return result;
}

static prMALError
ImporterGetSubTypeNames(
    imStdParms     *stdParms,
    csSDK_int32 fileType,
    imSubTypeDescriptionRec **subTypeDescriptionRec)
{
    prMALError    result = malNoError;

    csSDK_int32 supportedFileFormat = reinterpret_cast<const csSDK_int32&>(CodecRegistry::codec()->details().fileFormat);

    if (fileType == supportedFileFormat)
    {
        *subTypeDescriptionRec = (imSubTypeDescriptionRec *)stdParms->piSuites->memFuncs->newPtrClear(sizeof(imSubTypeDescriptionRec));

        csSDK_int32 videoFormat = reinterpret_cast<const csSDK_int32&>(CodecRegistry::codec()->details().videoFormat);
        (*subTypeDescriptionRec)->subType = videoFormat;

        // should use the subtype format here, but we don't break out codec subtypes atm
		SDKStringConvert::to_buffer(CodecRegistry::codec()->details().fileFormatShortName, (*subTypeDescriptionRec)->subTypeName);
    }
    else
    {
        result = imBadFormatIndex;
    }
    return result;
}



prMALError
GetInfoAudio(
    ImporterLocalRec8H    ldataH,
    imFileInfoRec8        *SDKFileInfo8)
{
    prMALError returnValue = malNoError;

    if((*ldataH)->movieReader->hasAudio())
    {
        SDKFileInfo8->hasAudio                = kPrTrue;

        auto audioDef = (*ldataH)->movieReader->audioDef();

        SDKFileInfo8->audInfo.numChannels = audioDef.numChannels;
        SDKFileInfo8->audInfo.sampleRate = (float)audioDef.sampleRate;
        std::map<std::pair<int, AudioEncoding>, PrAudioSampleType> bpsAndSignToPrAudioSampleType{
            {{1, AudioEncoding_Signed_PCM}, kPrAudioSampleType_8BitTwosInt},
            {{2, AudioEncoding_Signed_PCM}, kPrAudioSampleType_16BitInt},
            {{4, AudioEncoding_Signed_PCM}, kPrAudioSampleType_32BitInt},
            {{1, AudioEncoding_Unsigned_PCM}, kPrAudioSampleType_8BitInt},
            {{2, AudioEncoding_Unsigned_PCM}, kPrAudioSampleType_16BitInt},
            {{4, AudioEncoding_Unsigned_PCM}, kPrAudioSampleType_32BitInt}
        }; // these are used for display purposes

        SDKFileInfo8->audInfo.sampleType = bpsAndSignToPrAudioSampleType[{audioDef.bytesPerSample, audioDef.encoding}];
        SDKFileInfo8->audDuration = (*ldataH)->movieReader->numAudioFrames();
    }
    else
    {
        SDKFileInfo8->hasAudio = kPrFalse;
    }
    return returnValue;
}


/* 
    Populate the imFileInfoRec8 structure describing this file instance
    to Premiere.  Check file validity, allocate any private instance data 
    to share between different calls.
*/
prMALError
ImporterGetInfo8(
    imStdParms       *stdParms, 
    imFileAccessRec8 *fileAccessInfo8, 
    imFileInfoRec8   *fileInfo8)
{
    prMALError                    result                = malNoError;
    ImporterLocalRec8H            ldataH                = NULL;

    // If Premiere Pro 2.0 / Elements 2.0 or later, specify sequential audio so we can avoid
    // audio conforming.  Otherwise, specify random access audio so that we are not sent
    // imResetSequentialAudio and imGetSequentialAudio (which are not implemented in this sample)
    if (stdParms->imInterfaceVer >= IMPORTMOD_VERSION_6)
    {
        fileInfo8->accessModes = kSeparateSequentialAudio;
    }
    else
    {
        fileInfo8->accessModes = kRandomAccessImport;
    }

    fileInfo8->vidInfo.supportsAsyncIO            = kPrTrue;
    fileInfo8->vidInfo.supportsGetSourceVideo    = kPrTrue;
    fileInfo8->vidInfo.hasPulldown               = kPrFalse;
    fileInfo8->hasDataRate                       = kPrFalse; //!!! should be able to do thiskPrTrue;

    // Get a handle to our private data.  If it doesn't exist, allocate one
    // so we can use it to store our file instance info
    //!!!if(fileInfo8->privatedata)
    //!!!{

    //!!! presumably we went down a codepath that opened the file; otherwise none of the rest of the function would have made sense anyway
    ldataH = reinterpret_cast<ImporterLocalRec8H>(fileInfo8->privatedata);

    //!!!}
    //!!!else
    //!!!{
    //!!!    ldataH                 = reinterpret_cast<ImporterLocalRec8H>(stdParms->piSuites->memFuncs->newHandle(sizeof(ImporterLocalRec8)));
    //!!!    fileInfo8->privatedata = reinterpret_cast<void*>(ldataH);
    //!!!}

    // Either way, lock it in memory so it doesn't move while we modify it.
    stdParms->piSuites->memFuncs->lockHandle(reinterpret_cast<char**>(ldataH));

    // Acquire needed suites
    (*ldataH)->adobe = std::make_unique<AdobeImporterAPI>(stdParms->piSuites);

    // Get video info from header
    fileInfo8->hasVideo = kPrTrue;
    fileInfo8->vidInfo.subType     = (const csSDK_int32 &)(CodecRegistry::codec()->details().videoFormat);
    fileInfo8->vidInfo.imageWidth  = (*ldataH)->movieReader->width();
    fileInfo8->vidInfo.imageHeight = (*ldataH)->movieReader->height();
    fileInfo8->vidInfo.depth       = 32;  //!!! lack of alpha should change this
    fileInfo8->vidInfo.fieldType   = prFieldsNone;

    fileInfo8->vidInfo.alphaType = alphaStraight;

    fileInfo8->vidInfo.pixelAspectNum = 1;
    fileInfo8->vidInfo.pixelAspectDen = 1;
    fileInfo8->vidScale               = (csSDK_int32)(*ldataH)->movieReader->frameRateNumerator();
    fileInfo8->vidSampleSize          = (csSDK_int32)(*ldataH)->movieReader->frameRateDenominator();
    fileInfo8->vidDuration            = (int32_t)((*ldataH)->movieReader->numFrames() * fileInfo8->vidSampleSize);

    // Get audio info from header
    result = GetInfoAudio(ldataH, fileInfo8);

    stdParms->piSuites->memFuncs->unlockHandle(reinterpret_cast<char**>(ldataH));

    return result;
}

template<typename T, typename CHANNELS_T, typename NSAMPLES_T>
static void deinterleave(const T* source, CHANNELS_T nChannels, NSAMPLES_T nSamples, float **dest)
{
    auto constexpr half_range{ 0.5 * (size_t(std::numeric_limits<T>::max()) - std::numeric_limits<T>::min()) };
    auto constexpr scale{ 1.0 / half_range };
    auto constexpr offset{ -scale * std::numeric_limits<T>::min() - 1 };

    for (auto channel = 0; channel != nChannels; ++channel)
    {
        const T* s = &source[channel];
        float *d = dest[channel];
        for (int i = 0; i < nSamples; ++i) {
            *d = float(*s * scale + offset);
            s += nChannels;
            ++d;
        }
    }
}

static prMALError
ImporterImportAudio7(
    imStdParms *stdParms,
    imFileRef   ,
    imImportAudioRec7 *importAudioRec7)
{
    prMALError result = malNoError;
    ImporterLocalRec8H ldataH = NULL;
    ldataH = reinterpret_cast<ImporterLocalRec8H>(importAudioRec7->privateData);
    
    // !!! near end we're asked to read beyond the duration we provided in GetInfoAudio :(
    // !!! don't fail in that case
    auto numFrames = (*ldataH)->movieReader->numAudioFrames();
    auto safePosition = std::min(importAudioRec7->position, numFrames);
    auto safeSize = std::min((int64_t)importAudioRec7->size, numFrames - safePosition);

    std::vector<uint8_t> buffer;
    (*ldataH)->movieReader->readAudio(safePosition, safeSize, buffer);

    auto audioDef = (*ldataH)->movieReader->audioDef();
    int numChannels = audioDef.numChannels;
    int bytesPerSample = audioDef.bytesPerSample;
    AudioEncoding encoding = audioDef.encoding;

    if ((bytesPerSample == 1) && (encoding == AudioEncoding_Signed_PCM)) {
        deinterleave((int8_t*)&buffer[0], numChannels, safeSize, importAudioRec7->buffer);
    }
    else if ((bytesPerSample == 2) && (encoding == AudioEncoding_Signed_PCM)) {
        deinterleave((int16_t*)&buffer[0], numChannels, safeSize, importAudioRec7->buffer);
    }
    else if ((bytesPerSample == 4) && (encoding == AudioEncoding_Signed_PCM)) {
        deinterleave((int32_t*)&buffer[0], numChannels, safeSize, importAudioRec7->buffer);
    }
    else if ((bytesPerSample == 1) && (encoding == AudioEncoding_Unsigned_PCM)) {
        deinterleave((uint8_t*)&buffer[0], numChannels, safeSize, importAudioRec7->buffer);
    }
    else if ((bytesPerSample == 2) && (encoding == AudioEncoding_Unsigned_PCM)) {
        deinterleave((uint16_t*)&buffer[0], numChannels, safeSize, importAudioRec7->buffer);
    }
    else if ((bytesPerSample == 4) && (encoding == AudioEncoding_Unsigned_PCM)) {
        deinterleave((uint32_t*)&buffer[0], numChannels, safeSize, importAudioRec7->buffer);
    }

    // !!! assign 0's for dud read
    if (importAudioRec7->size > safeSize)
    {
        for (size_t iChannel = 0; iChannel < numChannels; ++iChannel) {
            auto channel = importAudioRec7->buffer[iChannel];
            std::fill(channel + safeSize, channel + importAudioRec7->size, 0.0f);
        }
    }
    // !!!

    return result;
}


#if 0
static prMALError 
SDKPreferredFrameSize(
    imStdParms                    *stdparms, 
    imPreferredFrameSizeRec        *preferredFrameSizeRec)
{
    prMALError            result    = malNoError;
    ImporterLocalRec8H    ldataH    = reinterpret_cast<ImporterLocalRec8H>(preferredFrameSizeRec->inPrivateData);

    // Enumerate formats in order of priority, starting from the most preferred format
    switch(preferredFrameSizeRec->inIndex)
    {
        case 0:
            preferredFrameSizeRec->outWidth = (*ldataH)->theFile.width;
            preferredFrameSizeRec->outHeight = (*ldataH)->theFile.height;
            // If we supported more formats, we'd return imIterateFrameSizes to request to be called again
            result = malNoError;
            break;
    
        default:
            // We shouldn't be called for anything other than the case above
            result = imOtherErr;
    }

    return result;
}
#endif

static prMALError 
ImporterGetSourceVideo(
    imStdParms          *stdparms, 
    imFileRef            , 
    imSourceVideoRec    *sourceVideoRec)
{
    //!!!prMALError        result      = malNoError;
    imFrameFormat    *frameFormat;
    char             *frameBuffer;

    // Get the privateData handle you stored in imGetInfo
    ImporterLocalRec8H ldataH = reinterpret_cast<ImporterLocalRec8H>(sourceVideoRec->inPrivateData);

    PrTime ticksPerSecond = 0;
    (*ldataH)->adobe->TimeSuite->GetTicksPerSecond(&ticksPerSecond);
    int64_t num = sourceVideoRec->inFrameTime * (*ldataH)->movieReader->frameRateNumerator();
    int64_t den = ticksPerSecond * (*ldataH)->movieReader->frameRateDenominator();
    int32_t iFrame = (int32_t)(num / den);


    //!!! cache usually returns 'true', even for frames that haven't been added to it yet
    //!!! !!! this is probably due to the importerID being incorrect, as it was previously
    //!!! !!! being set GetInfo8 from uninitialised data; should probably be set at FileOpen
    //!!! !!!
    //!!!
    //!!! // Check to see if frame is already in cache
    //!!!
    //!!! result = (*ldataH)->PPixCacheSuite->GetFrameFromCache((*ldataH)->importerID,
    //!!!                                                         0,
    //!!!                                                         iFrame,
    //!!!                                                         1,
    //!!!                                                         sourceVideoRec->inFrameFormats,
    //!!!                                                         sourceVideoRec->outFrame,
    //!!!                                                         NULL,
    //!!!                                                         NULL);

    // If frame is not in the cache, read the frame and put it in the cache; otherwise, we're done
    //!!! if (result != suiteError_NoError)
    {
        // Get parameters for ReadFrameToBuffer()
        frameFormat = &sourceVideoRec->inFrameFormats[0];
        prRect rect;
        if (frameFormat->inFrameWidth == 0 && frameFormat->inFrameHeight == 0)
        {
            frameFormat->inFrameWidth = (*ldataH)->movieReader->width();
            frameFormat->inFrameHeight = (*ldataH)->movieReader->height();
        }
        // Windows and MacOS have different definitions of Rects, so use the cross-platform prSetRect
        prSetRect (&rect, 0, 0, frameFormat->inFrameWidth, frameFormat->inFrameHeight);
        (*ldataH)->adobe->PPixCreatorSuite->CreatePPix(sourceVideoRec->outFrame, PrPPixBufferAccess_ReadWrite, frameFormat->inPixelFormat, &rect);
        (*ldataH)->adobe->PPixSuite->GetPixels(*sourceVideoRec->outFrame, PrPPixBufferAccess_ReadWrite, &frameBuffer);

        (*ldataH)->movieReader->readVideoFrame(iFrame, (*ldataH)->readBuffer);
        (*ldataH)->decoderJob->decode((*ldataH)->readBuffer);

        uint8_t *bgraBottomLeftOrigin = (uint8_t *)frameBuffer;
        int32_t stride;

        // If extra row padding is needed, add it
        (*ldataH)->adobe->PPixSuite->GetRowBytes(*sourceVideoRec->outFrame, &stride);

        (*ldataH)->decoderJob->copyLocalToExternal(bgraBottomLeftOrigin, stride);

        //!!! (*ldataH)->PPixCacheSuite->AddFrameToCache((*ldataH)->importerID,
        //!!!                                             0,
        //!!!                                             *sourceVideoRec->outFrame,
        //!!!                                             iFrame,
        //!!!                                             NULL,
        //!!!                                            NULL);

        return malNoError;
    }

    //!!! return result;
}

static prMALError
ImporterCreateAsyncImporter(
    imStdParms					*stdparms,
    imAsyncImporterCreationRec	*asyncImporterCreationRec)
{
    prMALError		result = malNoError;

    // Set entry point for async importer
    asyncImporterCreationRec->outAsyncEntry = xAsyncImportEntry;

    // Create and initialize async importer
    // Deleted during aiClose
    auto ldata = (*reinterpret_cast<ImporterLocalRec8H>(asyncImporterCreationRec->inPrivateData));
    const auto codec = *CodecRegistry::codec();
    auto fileFormat = codec.details().fileFormat;
    auto movieReader = createMovieReader(fileFormat, ldata->filePath);
    auto width = movieReader->width();
    auto height = movieReader->height();
    auto numerator = movieReader->frameRateNumerator();
    auto denominator = movieReader->frameRateDenominator();
  
    AsyncImporter *asyncImporter = new AsyncImporter(
        std::make_unique<AdobeImporterAPI>(stdparms->piSuites),
        std::make_unique<Importer>(std::move(movieReader), std::move(ldata->decoder)),
        width, height,
        numerator, denominator
    );

    // Store importer as private data
    asyncImporterCreationRec->outAsyncPrivateData = reinterpret_cast<void*>(asyncImporter);
    return result;
}

PREMPLUGENTRY DllExport xImportEntry (
    csSDK_int32      selector,
    imStdParms      *stdParms, 
    void            *param1, 
    void            *param2)
{
    FDN_DEBUG("xImportEntry selector=", selector);

    prMALError result = imUnsupported;

    try {
    static bool noImporter = (!CodecRegistry::codec()->createDecoder);
    if (noImporter)
        return result;

        switch (selector)
        {
        case imInit:
            FDN_DEBUG("imInit");
            FDN_INFO("initialising importer");
            result = ImporterInit(stdParms,
                reinterpret_cast<imImportInfoRec*>(param1));
            break;

            // To be demonstrated
            // case imShutdown:

        case imGetPrefs8:
            FDN_DEBUG("imGetPrefs8");
            result = ImporterGetPrefs8(stdParms,
                reinterpret_cast<imFileAccessRec8*>(param1),
                reinterpret_cast<imGetPrefsRec*>(param2));
            break;

            // To be demonstrated
            // case imSetPrefs:

        case imGetInfo8:
            FDN_DEBUG("imGetInfo8");
            result = ImporterGetInfo8(stdParms,
                reinterpret_cast<imFileAccessRec8*>(param1),
                reinterpret_cast<imFileInfoRec8*>(param2));
            break;

        case imImportAudio7:
            FDN_DEBUG("imImportAudio7");
            result = ImporterImportAudio7(stdParms,
                reinterpret_cast<imFileRef>(param1),
                reinterpret_cast<imImportAudioRec7*>(param2));
            break;

        case imOpenFile8:
            FDN_DEBUG("imOpenFile8");
            result = ImporterOpenFile8(stdParms,
                reinterpret_cast<imFileRef*>(param1),
                reinterpret_cast<imFileOpenRec8*>(param2));
            break;

        case imQuietFile:
            FDN_DEBUG("imQuietFile");
            result = ImporterQuietFile(stdParms,
                reinterpret_cast<imFileRef*>(param1),
                param2);
            break;

        case imCloseFile:
            FDN_DEBUG("imCloseFile");
            result = ImporterCloseFile(stdParms,
                reinterpret_cast<imFileRef*>(param1),
                param2);
            break;

        case imGetTimeInfo8:
            FDN_DEBUG("imGetTimeInfo8");
            result = imUnsupported;
            break;

        case imSetTimeInfo8:
            FDN_DEBUG("imSetTimeInfo8");
            result = imUnsupported;
            break;

        case imAnalysis:
            FDN_DEBUG("imAnalysis");
            //!!!            result = ImporterAnalysis(stdParms,
            //!!!                                      reinterpret_cast<imFileRef>(param1),
            //!!!                                      reinterpret_cast<imAnalysisRec*>(param2));
            result = imUnsupported;
            break;

        case imDataRateAnalysis:
            FDN_DEBUG("imDataRateAnalysis");
            //!!!            result = ImporterDataRateAnalysis(stdParms,
            //!!!                                              reinterpret_cast<imFileRef>(param1),
            //!!!                                              reinterpret_cast<imDataRateAnalysisRec*>(param2));
            result = imUnsupported;
            break;

        case imGetIndFormat:
            FDN_DEBUG("imGetIndFormat");
            result = ImporterGetIndFormat(stdParms,
                reinterpret_cast<csSDK_size_t>(param1),
                reinterpret_cast<imIndFormatRec*>(param2));
            break;

        case imGetSubTypeNames:
            FDN_DEBUG("imGetSubTypeNames");
            result = ImporterGetSubTypeNames(stdParms,
                reinterpret_cast<csSDK_size_t>(param1),
                reinterpret_cast<imSubTypeDescriptionRec**>(param2));
            break;

        case imSaveFile8:
            FDN_DEBUG("imSaveFile8");
            result = imUnsupported;
            break;

        case imDeleteFile8:
            FDN_DEBUG("imDeleteFile8");
            result = imUnsupported;
            break;

        case imGetMetaData:
            FDN_DEBUG("imGetMetaData");
            result = imUnsupported;
            break;

        case imSetMetaData:
            FDN_DEBUG("imSetMetaData");
            result = imUnsupported;
            break;

        case imGetIndPixelFormat:
            FDN_DEBUG("imGetIndPixelFormat");
            result = ImporterGetIndPixelFormat(stdParms,
                reinterpret_cast<csSDK_size_t>(param1),
                reinterpret_cast<imIndPixelFormatRec*>(param2));
            break;

            // Importers that support the Premiere Pro 2.0 API must return malSupports8 for this selector
        case imGetSupports8:
            FDN_DEBUG("imGetSupports8");
            result = malSupports8;
            break;

        case imCheckTrim8:
            FDN_DEBUG("imCheckTrim8");
            result = imUnsupported;
            break;

        case imTrimFile8:
            FDN_DEBUG("imTrimFile8");
            result = imUnsupported;
            break;

        case imCalcSize8:
            FDN_DEBUG("imCalcSize8");
            result = imUnsupported;
            break;

        case imGetPreferredFrameSize:
            FDN_DEBUG("imGetPreferredFrameSize");
            //!!!            result = ImporterPreferredFrameSize(stdParms,
            //!!!                                                reinterpret_cast<imPreferredFrameSizeRec*>(param1));
            result = imUnsupported;
            break;

        case imGetSourceVideo:
            FDN_DEBUG("imGetSourceVideo");
            result = ImporterGetSourceVideo(stdParms,
                reinterpret_cast<imFileRef>(param1),
                reinterpret_cast<imSourceVideoRec*>(param2));
            break;

        case imCreateAsyncImporter:
            FDN_DEBUG("imCreateAsyncImporter");
            result = ImporterCreateAsyncImporter(stdParms,
                reinterpret_cast<imAsyncImporterCreationRec*>(param1));
            break;
        }
    }
    catch (const std::exception& ex)
    {
        FDN_ERROR("import error: ", ex.what());

        result = malUnknownError;
    }
    catch (...)
    {
        FDN_ERROR("unknown import error");

        result = malUnknownError;
    }

    return result;
}

