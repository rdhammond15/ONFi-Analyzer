#ifndef NANDFLASH_ANALYZER_H
#define NANDFLASH_ANALYZER_H

#include <Analyzer.h>
#include "NANDFlashAnalyzerResults.h"
#include "NANDFlashSimulationDataGenerator.h"

// Frame Types
enum OnfiFrameType
{
	Command,
	Address,
	Write,
	Read,
	Undefined
};

class NANDFlashAnalyzerSettings;
class ANALYZER_EXPORT NANDFlashAnalyzer : public Analyzer2
{
public:
	NANDFlashAnalyzer();
	virtual ~NANDFlashAnalyzer();

	virtual void SetupResults();
	virtual void WorkerThread();

	virtual U32 GenerateSimulationData(U64 newest_sample_requested, U32 sample_rate, SimulationChannelDescriptor **simulation_channels);
	virtual U32 GetMinimumSampleRateHz();

	virtual const char *GetAnalyzerName() const;
	virtual bool NeedsRerun();

protected: // Functions
	void Setup(void);
	void AdvanceToReadOrWriteEnableEdge(void);
	void AdvanceToReadEnableHighEdge(void);
	void AdvanceToWriteEnableHighEdge(void);
	void SynchronizeAllChannels(U64 sample_number);
	void GetByte(void);

protected: // vars
#pragma warning(push)
#pragma warning(disable : 4251) // warning C4251: 'SerialAnalyzer::<...>' : class <...> needs to have dll-interface to be used by clients of class
	std::unique_ptr<NANDFlashAnalyzerSettings> mSettings;
	std::unique_ptr<NANDFlashAnalyzerResults> mResults;

	AnalyzerChannelData *mCLE;
	AnalyzerChannelData *mALE;
	AnalyzerChannelData *mCE;
	AnalyzerChannelData *mReadEnable;
	AnalyzerChannelData *mWriteEnable;
	AnalyzerChannelData *mData[8];

	NANDFlashSimulationDataGenerator mSimulationDataGenerator;
	bool mSimulationInitilized;

	bool mMoreWriteTransitions;
	bool mMoreReadTransitions;
	bool mDataIsOutput;
	U8 mLastCommand;

#pragma warning(pop)
};

extern "C" ANALYZER_EXPORT const char *__cdecl GetAnalyzerName();
extern "C" ANALYZER_EXPORT Analyzer *__cdecl CreateAnalyzer();
extern "C" ANALYZER_EXPORT void __cdecl DestroyAnalyzer(Analyzer *analyzer);

#endif // NANDFLASH_ANALYZER_H
