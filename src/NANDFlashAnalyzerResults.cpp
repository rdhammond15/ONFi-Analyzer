#include "NANDFlashAnalyzerResults.h"
#include <AnalyzerHelpers.h>
#include "NANDFlashAnalyzer.h"
#include "NANDFlashAnalyzerSettings.h"
#include <iostream>
#include <fstream>

NANDFlashAnalyzerResults::NANDFlashAnalyzerResults(NANDFlashAnalyzer *analyzer, NANDFlashAnalyzerSettings *settings)
	: AnalyzerResults(),
	  mSettings(settings),
	  mAnalyzer(analyzer)
{
}

NANDFlashAnalyzerResults::~NANDFlashAnalyzerResults()
{
}

void NANDFlashAnalyzerResults::GenerateBubbleText(U64 frame_index, Channel &channel, DisplayBase display_base)
{
	ClearResultStrings();
	Frame frame = GetFrame(frame_index);

	char number_str[128];
	AnalyzerHelpers::GetNumberString(frame.mData1, display_base, 8, number_str, 128);

	switch(frame.mType) {
		case Command:
			AddResultString("CMD: ", number_str);
			break;
		case Address:
			AddResultString("ADDR: ", number_str);
			break;
		case Read:
			AddResultString("RD: ", number_str);
			break;
		case Write:
			AddResultString("WR: ", number_str);
			break;
		case Undefined:
		default:
			AddResultString("UNDEF: ", number_str);
			break;
	}
}

void NANDFlashAnalyzerResults::GenerateExportFile(const char *file, DisplayBase display_base, U32 export_type_user_id)
{
	std::ofstream file_stream(file, std::ios::out);

	U64 trigger_sample = mAnalyzer->GetTriggerSample();
	U32 sample_rate = mAnalyzer->GetSampleRate();

	U64 num_frames = GetNumFrames();
	for (U32 i = 0; i < num_frames; i++)
	{
		Frame frame = GetFrame(i);

		char number_str[128];
		AnalyzerHelpers::GetNumberString(frame.mData1, display_base, 8, number_str, 128);
		switch (frame.mType)
		{
		case Command:
			file_stream << "Cmd";
			break;
		case Address:
			file_stream << "Addr";
			break;
		case Read:
			file_stream << "Read";
			break;
		case Write:
			file_stream << "Write";
			break;
		case Undefined:
		default:
			file_stream << "Undef";
			break;
		}

		file_stream << ",";
		file_stream << number_str << std::endl;

		if (UpdateExportProgressAndCheckForCancel(i, num_frames) == true)
		{
			file_stream.close();
			return;
		}
	}

	file_stream.close();
}

void NANDFlashAnalyzerResults::GenerateFrameTabularText(U64 frame_index, DisplayBase display_base)
{
#ifdef SUPPORTS_PROTOCOL_SEARCH
	Frame frame = GetFrame(frame_index);
	ClearTabularText();

	char number_str[128];
	AnalyzerHelpers::GetNumberString(frame.mData1, display_base, 8, number_str, 128);
	AddTabularText(number_str);
#endif
}

void NANDFlashAnalyzerResults::GeneratePacketTabularText(U64 packet_id, DisplayBase display_base)
{
	// not supported
}

void NANDFlashAnalyzerResults::GenerateTransactionTabularText(U64 transaction_id, DisplayBase display_base)
{
	// not supported
}