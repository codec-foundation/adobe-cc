#include "codec_registration.hpp"
#include "configure.hpp"
#include "export_settings.hpp"
#include "premiereParams.hpp"
#include "prstring.hpp"
#include "string_conversion.hpp"

const int k_chunkingMin = 1;
const int k_chunkingMax = 64;

prMALError generateDefaultParams(exportStdParms *stdParms, exGenerateDefaultParamRec *generateDefaultParamRec)
{
    prMALError result = malNoError;
    ExportSettings* settings = reinterpret_cast<ExportSettings*>(generateDefaultParamRec->privateData);
    PrSDKExportParamSuite* exportParamSuite = settings->exportParamSuite;
    PrSDKExportInfoSuite* exportInfoSuite = settings->exportInfoSuite;
    PrSDKTimeSuite* timeSuite = settings->timeSuite;
    csSDK_int32	exporterPluginID = generateDefaultParamRec->exporterPluginID;
    csSDK_int32	mgroupIndex = 0;
    PrParam	hasVideo,
        hasAudio,
        seqWidth,
        seqHeight,
        seqFrameRate,
        seqChannelType,
        seqSampleRate;


    // !!! WORKAROUND - needed in CC 2020 at least
    // !!! These parameters aren't used by any foundation-based codecs, but if they're not present the match-source button
    // !!! causes presets to become unusable
    PrParam pixelAspectRatioNumerator;
    PrParam pixelAspectRatioDenominator;
    PrParam fieldTypeP;
    // !!! end workaround


    const auto& codec = *CodecRegistry::codec();

    if (exportInfoSuite)
    {
        exportInfoSuite->GetExportSourceInfo(exporterPluginID, kExportInfo_SourceHasVideo, &hasVideo);
        exportInfoSuite->GetExportSourceInfo(exporterPluginID, kExportInfo_SourceHasAudio, &hasAudio);
        exportInfoSuite->GetExportSourceInfo(exporterPluginID, kExportInfo_VideoWidth, &seqWidth);
        exportInfoSuite->GetExportSourceInfo(exporterPluginID, kExportInfo_VideoHeight, &seqHeight);
        // !!! WORKAROUND - needed in CC 2020 at least
        // !!! These parameters aren't used by any foundation-based codecs, but if they're not present the match-source button
        // !!! causes presets to become unusable
        exportInfoSuite->GetExportSourceInfo(exporterPluginID, kExportInfo_PixelAspectNumerator, &pixelAspectRatioNumerator);
        exportInfoSuite->GetExportSourceInfo(exporterPluginID, kExportInfo_PixelAspectDenominator, &pixelAspectRatioDenominator);
        exportInfoSuite->GetExportSourceInfo(exporterPluginID, kExportInfo_VideoFieldType, &fieldTypeP);
        // !!! end workaround

        if (seqWidth.mInt32 == 0)
            seqWidth.mInt32 = 1920;

        if (seqHeight.mInt32 == 0)
            seqHeight.mInt32 = 1080;

        exportInfoSuite->GetExportSourceInfo(exporterPluginID, kExportInfo_VideoFrameRate, &seqFrameRate);
        exportInfoSuite->GetExportSourceInfo(exporterPluginID, kExportInfo_AudioChannelsType, &seqChannelType);
        exportInfoSuite->GetExportSourceInfo(exporterPluginID, kExportInfo_AudioSampleRate, &seqSampleRate);
    }

    if (exportParamSuite)
    {
        exportParamSuite->AddMultiGroup(exporterPluginID, &mgroupIndex);
        exportParamSuite->AddParamGroup(exporterPluginID, mgroupIndex, ADBETopParamGroup, ADBEVideoTabGroup, StringForPr(TOP_VIDEO_PARAM_GROUP_NAME), kPrFalse, kPrFalse, kPrFalse);
        exportParamSuite->AddParamGroup(exporterPluginID, mgroupIndex, ADBEVideoTabGroup, ADBEVideoCodecGroup, StringForPr(VIDEO_CODEC_PARAM_GROUP_NAME), kPrFalse, kPrFalse, kPrFalse);
        exportParamSuite->AddParamGroup(exporterPluginID, mgroupIndex, ADBEVideoTabGroup, ADBEBasicVideoGroup, StringForPr(BASIC_VIDEO_PARAM_GROUP_NAME), kPrFalse, kPrFalse, kPrFalse);

        exportParamSuite->AddParamGroup(exporterPluginID, mgroupIndex, ADBEVideoTabGroup, codec.details().premiereGroupName.c_str(), StringForPr(CODEC_SPECIFIC_PARAM_GROUP_NAME), kPrFalse, kPrFalse, kPrFalse);
        exNewParamInfo widthParam;
        exParamValues widthValues;
		SDKStringConvert::to_buffer(ADBEVideoWidth, widthParam.identifier);
        widthParam.paramType = exParamType_int;
        widthParam.flags = exParamFlag_none;
        widthValues.rangeMin.intValue = 16;
        widthValues.rangeMax.intValue = 16384;
        widthValues.value.intValue = seqWidth.mInt32;
        widthValues.disabled = kPrFalse;
        widthValues.hidden = kPrFalse;
        widthParam.paramValues = widthValues;
        exportParamSuite->AddParam(exporterPluginID, mgroupIndex, ADBEBasicVideoGroup, &widthParam);

        exNewParamInfo heightParam;
        exParamValues heightValues;
		SDKStringConvert::to_buffer(ADBEVideoHeight, heightParam.identifier);
        heightParam.paramType = exParamType_int;
        heightParam.flags = exParamFlag_none;
        heightValues.rangeMin.intValue = 16;
        heightValues.rangeMax.intValue = 16384;
        heightValues.value.intValue = seqHeight.mInt32;
        heightValues.disabled = kPrFalse;
        heightValues.hidden = kPrFalse;
        heightParam.paramValues = heightValues;
        exportParamSuite->AddParam(exporterPluginID, mgroupIndex, ADBEBasicVideoGroup, &heightParam);
        if (codec.details().hasExplicitIncludeAlphaChannel)
        {
            exNewParamInfo includeAlphaParam;
            exParamValues includeAlphaValues;
			SDKStringConvert::to_buffer(codec.details().premiereIncludeAlphaChannelName, includeAlphaParam.identifier);
            includeAlphaParam.paramType = exParamType_bool;
            includeAlphaParam.flags = exParamFlag_none;
            includeAlphaValues.rangeMin.intValue = 0;
            includeAlphaValues.rangeMax.intValue = 1;
            includeAlphaValues.value.intValue = 0;
            includeAlphaValues.disabled = kPrFalse;
            includeAlphaValues.hidden = kPrFalse;
            includeAlphaValues.disabled = kPrFalse;
            includeAlphaParam.paramValues = includeAlphaValues;
            exportParamSuite->AddParam(exporterPluginID, mgroupIndex, ADBEBasicVideoGroup, &includeAlphaParam);
        }

        if (codec.details().subtypes.size())
        {
            exNewParamInfo hapSubcodecParam;
            exParamValues hapSubcodecValues;
			SDKStringConvert::to_buffer(ADBEVideoCodec, hapSubcodecParam.identifier);
            hapSubcodecParam.paramType = exParamType_int;
            hapSubcodecParam.flags = exParamFlag_none;
            hapSubcodecValues.rangeMin.intValue = 0;
            hapSubcodecValues.rangeMax.intValue = 4;
            auto temp = codec.details().defaultSubType;
            hapSubcodecValues.value.intValue = reinterpret_cast<int32_t&>(temp); //!!! seqHapSubcodec.mInt32;
            hapSubcodecValues.disabled = kPrFalse;
            hapSubcodecValues.hidden = kPrFalse;
            hapSubcodecParam.paramValues = hapSubcodecValues;
            exportParamSuite->AddParam(exporterPluginID, mgroupIndex, ADBEBasicVideoGroup, &hapSubcodecParam);
        }

        exNewParamInfo frameRateParam;
        exParamValues frameRateValues;
        SDKStringConvert::to_buffer(ADBEVideoFPS, frameRateParam.identifier);
        frameRateParam.paramType = exParamType_ticksFrameRate;
        frameRateParam.flags = exParamFlag_none;
        frameRateValues.rangeMin.timeValue = 1;
        timeSuite->GetTicksPerSecond(&frameRateValues.rangeMax.timeValue);
        frameRateValues.value.timeValue = seqFrameRate.mInt64;
        frameRateValues.disabled = kPrFalse;
        frameRateValues.hidden = kPrFalse;
        frameRateParam.paramValues = frameRateValues;
        exportParamSuite->AddParam(exporterPluginID, mgroupIndex, ADBEBasicVideoGroup, &frameRateParam);

        // !!! WORKAROUND - needed in CC 2020 at least
        // !!! These parameters aren't used by any foundation-based codecs, but if they're not present the match-source button
        // !!! causes presets to become unusable
        
        // pixel aspect ratio
        exParamValues parValues;
        parValues.structVersion = 1;
        parValues.rangeMin.ratioValue.numerator = 10;
        parValues.rangeMin.ratioValue.denominator = 11;
        parValues.rangeMax.ratioValue.numerator = 2;
        parValues.rangeMax.ratioValue.denominator = 1;
        parValues.value.ratioValue.numerator = pixelAspectRatioNumerator.mInt32;
        parValues.value.ratioValue.denominator = pixelAspectRatioDenominator.mInt32;
        parValues.disabled = kPrFalse;
        parValues.hidden = kPrTrue;   // !!! because this is only present to avoid preset corruption

        exNewParamInfo parParam;
        parParam.structVersion = 1;
        strncpy(parParam.identifier, ADBEVideoAspect, 255);
        parParam.paramType = exParamType_ratio;
        parParam.flags = exParamFlag_none;
        parParam.paramValues = parValues;

        settings->exportParamSuite->AddParam(exporterPluginID, mgroupIndex, ADBEBasicVideoGroup, &parParam);

        // field order
        if (fieldTypeP.mInt32 == prFieldsUnknown)
            fieldTypeP.mInt32 = prFieldsNone;

        exParamValues fieldOrderValues;
        fieldOrderValues.structVersion = 1;
        fieldOrderValues.value.intValue = fieldTypeP.mInt32;
        fieldOrderValues.disabled = kPrFalse;
        fieldOrderValues.hidden = kPrTrue;  // !!! because this only present to avoid preset corruption

        exNewParamInfo fieldOrderParam;
        fieldOrderParam.structVersion = 1;
        strncpy(fieldOrderParam.identifier, ADBEVideoFieldType, 255);
        fieldOrderParam.paramType = exParamType_int;
        fieldOrderParam.flags = exParamFlag_none;
        fieldOrderParam.paramValues = fieldOrderValues;

        exportParamSuite->AddParam(exporterPluginID, mgroupIndex, ADBEBasicVideoGroup, &fieldOrderParam);

        // !!! end workaround
        if (codec.details().hasChunkCount) {
            exNewParamInfo chunkCountParam;
            exParamValues chunkCountValues;
			SDKStringConvert::to_buffer(codec.details().premiereChunkCountName, chunkCountParam.identifier);
            chunkCountParam.paramType = exParamType_int;
            chunkCountParam.flags = exParamFlag_optional;
            chunkCountValues.rangeMin.intValue = k_chunkingMin;
            chunkCountValues.rangeMax.intValue = k_chunkingMax;
            chunkCountValues.value.intValue = 1;
            chunkCountValues.disabled = kPrFalse;
            chunkCountValues.hidden = kPrFalse;
            chunkCountParam.paramValues = chunkCountValues;
            exportParamSuite->AddParam(exporterPluginID, mgroupIndex, codec.details().premiereGroupName.c_str(), &chunkCountParam);
        }

        if (codec.details().hasQualityForSubType(codec.details().defaultSubType))
        {
            exNewParamInfo qualityParam;
            exParamValues qualityValues;
			SDKStringConvert::to_buffer(ADBEVideoQuality, qualityParam.identifier);
            qualityParam.paramType = exParamType_int;
            qualityParam.flags = exParamFlag_none;

            auto qualities = codec.details().quality.descriptions;
            int worst = qualities.begin()->first;
            int best = qualities.rbegin()->first;

            qualityValues.rangeMin.intValue = worst;
            qualityValues.rangeMax.intValue = best;
            qualityValues.value.intValue = codec.details().quality.defaultQuality;
            qualityValues.disabled = kPrFalse;
            qualityValues.hidden = kPrFalse;
            qualityParam.paramValues = qualityValues;
            exportParamSuite->AddParam(exporterPluginID, mgroupIndex, ADBEBasicVideoGroup, &qualityParam);
        }

        // Audio parameters
        exportParamSuite->AddParamGroup(exporterPluginID, mgroupIndex, ADBETopParamGroup, ADBEAudioTabGroup, StringForPr(TOP_AUDIO_PARAM_GROUP_NAME), kPrFalse, kPrFalse, kPrFalse);
        exportParamSuite->AddParamGroup(exporterPluginID, mgroupIndex, ADBEAudioTabGroup, ADBEBasicAudioGroup, StringForPr(BASIC_AUDIO_PARAM_GROUP_NAME), kPrFalse, kPrFalse, kPrFalse);

        // Sample rate
        exNewParamInfo sampleRateParam;
        exParamValues sampleRateValues;
		SDKStringConvert::to_buffer(ADBEAudioRatePerSecond, sampleRateParam.identifier);
        sampleRateParam.paramType = exParamType_float;
        sampleRateParam.flags = exParamFlag_none;
        sampleRateValues.value.floatValue = 44100.0f; // disguise servers default samplerate
        sampleRateValues.disabled = kPrFalse;
        sampleRateValues.hidden = kPrFalse;
        sampleRateParam.paramValues = sampleRateValues;
        exportParamSuite->AddParam(exporterPluginID, mgroupIndex, ADBEBasicAudioGroup, &sampleRateParam);
        
        // Channel type
        exNewParamInfo channelTypeParam;
        exParamValues channelTypeValues;
		SDKStringConvert::to_buffer(ADBEAudioNumChannels, channelTypeParam.identifier);
        channelTypeParam.paramType = exParamType_int;
        channelTypeParam.flags = exParamFlag_none;
        channelTypeValues.value.intValue = kPrAudioChannelType_Stereo;
        channelTypeValues.disabled = kPrFalse; // TODO in Release disable to simplify user expirience: only stereo
        channelTypeValues.hidden = kPrFalse;
        channelTypeParam.paramValues = channelTypeValues;
        exportParamSuite->AddParam(exporterPluginID, mgroupIndex, ADBEBasicAudioGroup, &channelTypeParam);

        exportParamSuite->SetParamsVersion(exporterPluginID, codec.details().premiereParamsVersion);
    }

    return result;
}

prMALError postProcessParams(exportStdParms *stdParmsP, exPostProcessParamsRec *postProcessParamsRecP)
{
    const csSDK_uint32 exID = postProcessParamsRecP->exporterPluginID;
    ExportSettings* settings = reinterpret_cast<ExportSettings*>(postProcessParamsRecP->privateData);
    PrTime ticksPerSecond = 0;

    const auto& codec = *CodecRegistry::codec();

    exOneParamValueRec tempFrameRate;
    PrTime frameRates[] = { 10, 15, 23, 24, 25, 29, 30, 50, 59, 60 };
    PrTime frameRateNumDens[][2] = { { 10, 1 }, { 15, 1 }, { 24000, 1001 }, { 24, 1 }, { 25, 1 }, { 30000, 1001 }, { 30, 1 }, { 50, 1 }, { 60000, 1001 }, { 60, 1 } };

    exOneParamValueRec tempSampleRate;
    exOneParamValueRec tempQuality;
    float sampleRates[] = {44100.0f, 48000.0f};
    exOneParamValueRec tempChannelType;
    csSDK_int32 channelTypes[] = {kPrAudioChannelType_Mono, kPrAudioChannelType_Stereo, kPrAudioChannelType_51};

    const wchar_t* frameRateStrings[] = { STR_FRAME_RATE_10, STR_FRAME_RATE_15, STR_FRAME_RATE_23976, STR_FRAME_RATE_24, STR_FRAME_RATE_25, STR_FRAME_RATE_2997, STR_FRAME_RATE_30, STR_FRAME_RATE_50, STR_FRAME_RATE_5994, STR_FRAME_RATE_60 };
    const wchar_t *sampleRateStrings[] = {STR_SAMPLE_RATE_441, STR_SAMPLE_RATE_48};
    const wchar_t *channelTypeStrings[] = {STR_CHANNEL_TYPE_MONO, STR_CHANNEL_TYPE_STEREO, STR_CHANNEL_TYPE_51};


	settings->timeSuite->GetTicksPerSecond(&ticksPerSecond);
    for (csSDK_int32 i = 0; i < sizeof(frameRates) / sizeof(PrTime); i++)
        frameRates[i] = ticksPerSecond / frameRateNumDens[i][0] * frameRateNumDens[i][1];

    settings->exportParamSuite->SetParamName(exID, 1, ADBEVideoCodecGroup, StringForPr(VIDEO_CODEC_PARAM_GROUP_NAME));

    settings->exportParamSuite->SetParamName(exID, 0, ADBEVideoCodec, StringForPr(STR_CODEC));

    settings->exportParamSuite->SetParamDescription(exID, 0, ADBEVideoCodec, StringForPr(STR_CODEC_TOOLTIP));

    settings->exportParamSuite->SetParamName(exID, 0, ADBEBasicVideoGroup, StringForPr(BASIC_VIDEO_PARAM_GROUP_NAME));

    settings->exportParamSuite->SetParamName(exID, 0, ADBEVideoWidth, StringForPr(STR_WIDTH));

    settings->exportParamSuite->SetParamName(exID, 0, ADBEVideoHeight, StringForPr(STR_HEIGHT));

    // width
    exParamValues widthValues;
    settings->exportParamSuite->GetParamValue(exID, 0, ADBEVideoWidth, &widthValues);

    widthValues.rangeMin.intValue = 16;
    widthValues.rangeMax.intValue = 16384;

    settings->exportParamSuite->ChangeParam(exID, 0, ADBEVideoWidth, &widthValues);


    // height
    exParamValues heightValues;
    settings->exportParamSuite->GetParamValue(exID, 0, ADBEVideoHeight, &heightValues);

    heightValues.rangeMin.intValue = 16;
    heightValues.rangeMax.intValue = 16384;

    settings->exportParamSuite->ChangeParam(exID, 0, ADBEVideoHeight, &heightValues);


    // !!! WORKAROUND - needed in CC 2020 at least
    // !!! These parameters aren't used by any foundation-based codecs, but if they're not present the match-source button
    // !!! causes presets to become unusable

    // pixel aspect ratio
    settings->exportParamSuite->SetParamName(exID, 0, ADBEVideoAspect, StringForPr("Pixel Aspect Ratio"));

    csSDK_int32	PARs[][2] = { {1, 1}, {10, 11}, {40, 33}, {768, 702},
                            {1024, 702}, {2, 1}, {4, 3}, {3, 2} };

    const char* PARStrings[] = { "Square pixels (1.0)",
                                "D1/DV NTSC (0.9091)",
                                "D1/DV NTSC Widescreen 16:9 (1.2121)",
                                "D1/DV PAL (1.0940)",
                                "D1/DV PAL Widescreen 16:9 (1.4587)",
                                "Anamorphic 2:1 (2.0)",
                                "HD Anamorphic 1080 (1.3333)",
                                "DVCPRO HD (1.5)" };

    settings->exportParamSuite->ClearConstrainedValues(exID, 0, ADBEVideoAspect);

    exOneParamValueRec tempPAR;

    for (csSDK_int32 i = 0; i < sizeof(PARs) / sizeof(PARs[0]); i++)
    {
        tempPAR.ratioValue.numerator = PARs[i][0];
        tempPAR.ratioValue.denominator = PARs[i][1];
        settings->exportParamSuite->AddConstrainedValuePair(exID, 0, ADBEVideoAspect, &tempPAR, StringForPr(PARStrings[i]));
    }

    // field type
    settings->exportParamSuite->SetParamName(exID, 0, ADBEVideoFieldType, StringForPr("Field Type"));

    csSDK_int32	fieldOrders[] = { prFieldsUpperFirst,
                                    prFieldsLowerFirst,
                                    prFieldsNone };

    const char* fieldOrderStrings[] = { "Upper First",
                                        "Lower First",
                                        "None" };

    settings->exportParamSuite->ClearConstrainedValues(exID, 0, ADBEVideoFieldType);

    exOneParamValueRec tempFieldOrder;
    for (int i = 0; i < 3; i++)
    {
        tempFieldOrder.intValue = fieldOrders[i];
        settings->exportParamSuite->AddConstrainedValuePair(exID, 0, ADBEVideoFieldType, &tempFieldOrder, StringForPr(fieldOrderStrings[i]));
    }
    // !!! END WORKAROUND

    if (codec.details().subtypes.size())
    {
        exOneParamValueRec tempHapSubcodec;

        settings->exportParamSuite->SetParamName(exID, 0, ADBEVideoCodec, StringForPr(L"Subcodec type"));
        settings->exportParamSuite->ClearConstrainedValues(exID, 0, ADBEVideoCodec);
        const auto& subtypes = codec.details().subtypes;
        for (csSDK_int32 i = 0; i < subtypes.size(); i++) {
            const auto& subtype = subtypes[i];
            tempHapSubcodec.intValue = reinterpret_cast<const int32_t&>(subtype.first[0]);
            settings->exportParamSuite->AddConstrainedValuePair(exID, 0, ADBEVideoCodec, &tempHapSubcodec, StringForPr(subtype.second));
        }
    }

    settings->exportParamSuite->SetParamName(exID, 0, ADBEVideoFPS, StringForPr(STR_FRAME_RATE));
    settings->exportParamSuite->ClearConstrainedValues(exID, 0, ADBEVideoFPS);
    for (csSDK_int32 i = 0; i < sizeof(frameRates) / sizeof(PrTime); i++)
    {
        tempFrameRate.timeValue = frameRates[i];
        settings->exportParamSuite->AddConstrainedValuePair(exID, 0, ADBEVideoFPS, &tempFrameRate, StringForPr(frameRateStrings[i]));
    }

    if (codec.details().quality.hasQualityForAnySubType) {
        settings->exportParamSuite->SetParamName(exID, 0, ADBEVideoQuality, StringForPr(STR_QUALITY));
        auto qualities = codec.details().quality.descriptions;
        int worst = qualities.begin()->first;
        int best = qualities.rbegin()->first;

        exParamValues qualityValues;
        settings->exportParamSuite->GetParamValue(exID, 0, ADBEVideoQuality, &qualityValues);
        qualityValues.rangeMin.intValue = worst;
        qualityValues.rangeMax.intValue = best;
        qualityValues.disabled = kPrFalse;
        qualityValues.hidden = kPrFalse;
        settings->exportParamSuite->ChangeParam(exID, 0, ADBEVideoQuality, &qualityValues);

        settings->exportParamSuite->ClearConstrainedValues(exID, 0, ADBEVideoQuality);
        for (const auto& quality : qualities)
        {
            tempQuality.intValue = (csSDK_int32)quality.first;
            StringForPr qualityString(quality.second);
            settings->exportParamSuite->AddConstrainedValuePair(exID, 0, ADBEVideoQuality, &tempQuality, qualityString);
        }

        if (codec.details().subtypes.size()) {
            exParamValues subCodecTypeParam;
            settings->exportParamSuite->GetParamValue(exID, 0, ADBEVideoCodec, &subCodecTypeParam);
            const auto codecSubType = reinterpret_cast<Codec4CC&>(subCodecTypeParam.value.intValue);

            exParamValues qualityToValidate;
            bool enableQuality = codec.details().hasQualityForSubType(codecSubType);
            settings->exportParamSuite->GetParamValue(exID, 0, ADBEVideoQuality, &qualityToValidate);
            qualityToValidate.disabled = !enableQuality;
            settings->exportParamSuite->ChangeParam(exID, 0, ADBEVideoQuality, &qualityToValidate);
        }
    }
    
    if (codec.details().hasExplicitIncludeAlphaChannel)
    {
        settings->exportParamSuite->SetParamName(exID, 0, codec.details().premiereIncludeAlphaChannelName.c_str(), StringForPr(STR_INCLUDE_ALPHA));
    }

    settings->exportParamSuite->SetParamName(exID, 0, codec.details().premiereGroupName.c_str(), StringForPr(CODEC_SPECIFIC_PARAM_GROUP_NAME));

    if (codec.details().hasChunkCount) {
        settings->exportParamSuite->SetParamName(exID, 0, codec.details().premiereChunkCountName.c_str(), StringForPr(STR_CHUNKING));
        exParamValues chunkCountValues;
        settings->exportParamSuite->GetParamValue(exID, 0, codec.details().premiereChunkCountName.c_str(), &chunkCountValues);
        chunkCountValues.rangeMin.intValue = k_chunkingMin;
        chunkCountValues.rangeMax.intValue = k_chunkingMax;
        chunkCountValues.disabled = kPrFalse;
        chunkCountValues.hidden = kPrFalse;
        settings->exportParamSuite->ChangeParam(exID, 0, codec.details().premiereChunkCountName.c_str(), &chunkCountValues);
    }

    settings->exportParamSuite->SetParamName(exID, 0, ADBEBasicAudioGroup, StringForPr(BASIC_AUDIO_PARAM_GROUP_NAME));

    settings->exportParamSuite->SetParamName(exID, 0, ADBEAudioRatePerSecond, StringForPr(STR_SAMPLE_RATE));
    settings->exportParamSuite->ClearConstrainedValues(exID, 0, ADBEAudioRatePerSecond);
    for (csSDK_int32 i = 0; i < sizeof(sampleRates) / sizeof(float); i++)
    {
        tempSampleRate.floatValue = sampleRates[i];
        settings->exportParamSuite->AddConstrainedValuePair(exID, 0, ADBEAudioRatePerSecond, &tempSampleRate, StringForPr(sampleRateStrings[i]));
    }

    settings->exportParamSuite->SetParamName(exID, 0, ADBEAudioNumChannels, StringForPr(STR_CHANNEL_TYPE));
    settings->exportParamSuite->ClearConstrainedValues(exID, 0, ADBEAudioNumChannels);
    for (csSDK_int32 i = 0; i < sizeof(channelTypes) / sizeof(csSDK_int32); i++)
    {
        tempChannelType.intValue = channelTypes[i];
        settings->exportParamSuite->AddConstrainedValuePair(exID, 0, ADBEAudioNumChannels, &tempChannelType, StringForPr(channelTypeStrings[i]));
    }

    return malNoError;
}

prMALError getParamSummary(exportStdParms *stdParmsP, exParamSummaryRec *summaryRecP)
{
    wchar_t videoSummary[256], audioSummary[256];
    exParamValues width, height, includeAlphaChannel, frameRate, sampleRate, channelType;
    ExportSettings* settings = reinterpret_cast<ExportSettings*>(summaryRecP->privateData);
    PrSDKExportParamSuite* paramSuite = settings->exportParamSuite;
    PrSDKTimeSuite* timeSuite = settings->timeSuite;
    PrTime ticksPerSecond;
    const csSDK_int32 mgroupIndex = 0;
    const csSDK_int32 exporterPluginID = summaryRecP->exporterPluginID;

    const auto& codec = *CodecRegistry::codec();
    if (!paramSuite)
        return malNoError;

    paramSuite->GetParamValue(exporterPluginID, mgroupIndex, ADBEVideoWidth, &width);
    paramSuite->GetParamValue(exporterPluginID, mgroupIndex, ADBEVideoHeight, &height);
    bool hasExplicitUseAlphaChannel = codec.details().hasExplicitIncludeAlphaChannel;
    if (hasExplicitUseAlphaChannel)
    {
        paramSuite->GetParamValue(exporterPluginID, mgroupIndex, codec.details().premiereIncludeAlphaChannelName.c_str(), &includeAlphaChannel);
    }
    paramSuite->GetParamValue(exporterPluginID, mgroupIndex, ADBEVideoFPS, &frameRate);
    paramSuite->GetParamValue(exporterPluginID, mgroupIndex, ADBEAudioRatePerSecond, &sampleRate);
    paramSuite->GetParamValue(exporterPluginID, mgroupIndex, ADBEAudioNumChannels, &channelType);
    timeSuite->GetTicksPerSecond(&ticksPerSecond);

    //!!! this should be modified to read 'match source' if match source was selected
    //!!! but it's not clear how to obtain that information here
    swprintf(videoSummary, 256, L"%ix%i, %s%.2f fps",
            width.value.intValue, height.value.intValue,
            (hasExplicitUseAlphaChannel?(includeAlphaChannel.value.intValue ? L"with alpha, " : L"no alpha, "):L""),
            static_cast<float>(ticksPerSecond)
            / ((frameRate.value.timeValue != 0) ? static_cast<float>(frameRate.value.timeValue) : 1.0E50 // force result to zero if denom zero
              ));
	SDKStringConvert::to_buffer(videoSummary, summaryRecP->videoSummary);

    if (summaryRecP->exportAudio)
    {
        std::wstring audioChannelSummary;
        switch (channelType.value.intValue)
        {
        case kPrAudioChannelType_Mono:
            audioChannelSummary = STR_CHANNEL_TYPE_MONO;
            break;
        case kPrAudioChannelType_Stereo:
            audioChannelSummary = STR_CHANNEL_TYPE_STEREO;
            break;
        case kPrAudioChannelType_51:
            audioChannelSummary = STR_CHANNEL_TYPE_51;
            break;
        default:
            audioChannelSummary = L"Unknown";
        }

        swprintf(audioSummary, 256, L"Uncompressed, %.0f Hz, %ls, 16bit",
                 sampleRate.value.floatValue,
                 audioChannelSummary.c_str());
		SDKStringConvert::to_buffer(audioSummary, summaryRecP->audioSummary);
    }

    return malNoError;
}

prMALError paramButton(exportStdParms *stdParmsP, exParamButtonRec *getFilePrefsRecP)
{
    return malNoError;
}

prMALError validateParamChanged(exportStdParms *stdParmsP, exParamChangedRec *validateParamChangedRecP)
{
    ExportSettings* settings = reinterpret_cast<ExportSettings*>(validateParamChangedRecP->privateData);

    if (settings->exportParamSuite == nullptr)
        return exportReturn_ErrMemory;

    const csSDK_uint32 exID = validateParamChangedRecP->exporterPluginID;

    const auto& codec = *CodecRegistry::codec();

    if (codec.details().quality.hasQualityForAnySubType) {
        settings->exportParamSuite->SetParamName(exID, 0, ADBEVideoQuality, StringForPr(STR_QUALITY));
        auto qualities = codec.details().quality.descriptions;
        int worst = qualities.begin()->first;
        int best = qualities.rbegin()->first;

        exParamValues qualityValues;
        settings->exportParamSuite->GetParamValue(exID, 0, ADBEVideoQuality, &qualityValues);
        qualityValues.rangeMin.intValue = worst;
        qualityValues.rangeMax.intValue = best;
        qualityValues.disabled = kPrFalse;
        qualityValues.hidden = kPrFalse;
        settings->exportParamSuite->ChangeParam(exID, 0, ADBEVideoQuality, &qualityValues);

        if (codec.details().subtypes.size()) {
            exParamValues subCodecTypeParam;
            settings->exportParamSuite->GetParamValue(exID, 0, ADBEVideoCodec, &subCodecTypeParam);
            const auto codecSubType = reinterpret_cast<Codec4CC&>(subCodecTypeParam.value.intValue);

            exParamValues qualityToValidate;
            bool enableQuality = codec.details().hasQualityForSubType(codecSubType);
            settings->exportParamSuite->GetParamValue(exID, 0, ADBEVideoQuality, &qualityToValidate);
            qualityToValidate.disabled = !enableQuality;
            settings->exportParamSuite->ChangeParam(exID, 0, ADBEVideoQuality, &qualityToValidate);
        }
    }

    if (codec.details().hasChunkCount) {
        settings->exportParamSuite->SetParamName(exID, 0, codec.details().premiereChunkCountName.c_str(), StringForPr(STR_CHUNKING));
        exParamValues chunkCountValues;
        settings->exportParamSuite->GetParamValue(exID, 0, codec.details().premiereChunkCountName.c_str(), &chunkCountValues);
        chunkCountValues.rangeMin.intValue = k_chunkingMin;
        chunkCountValues.rangeMax.intValue = k_chunkingMax;
        chunkCountValues.disabled = kPrFalse;
        chunkCountValues.hidden = kPrFalse;
        settings->exportParamSuite->ChangeParam(exID, 0, codec.details().premiereChunkCountName.c_str(), &chunkCountValues);
    }

    return malNoError;
}
