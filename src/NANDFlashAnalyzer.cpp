#include "NANDFlashAnalyzer.h"
#include "NANDFlashAnalyzerSettings.h"
#include <AnalyzerChannelData.h>
#include <unordered_map>


NANDFlashAnalyzer::NANDFlashAnalyzer()
	: Analyzer2(),
	  mLastCommand(0x00),
	  mDataIsOutput(false),
	  mSettings(new NANDFlashAnalyzerSettings()),
	  mSimulationInitilized(false),
	  mCLE(NULL),
	  mALE(NULL),
	  mCE(NULL),
	  mReadEnable(NULL),
	  mWriteEnable(NULL),
	  mData(),
      mMoreReadTransitions(true),
      mMoreWriteTransitions(true)
{
	SetAnalyzerSettings(mSettings.get());
    UseFrameV2();
}

NANDFlashAnalyzer::~NANDFlashAnalyzer()
{
	KillThread();
}

void NANDFlashAnalyzer::SetupResults()
{
	mResults.reset(new NANDFlashAnalyzerResults(this, mSettings.get()));
	SetAnalyzerResults(mResults.get());

	if (mSettings->mReadEnableChannel != UNDEFINED_CHANNEL)
	{
		mResults->AddChannelBubblesWillAppearOn(mSettings->mReadEnableChannel);
	}
	if (mSettings->mWriteEnableChannel != UNDEFINED_CHANNEL)
	{
		mResults->AddChannelBubblesWillAppearOn(mSettings->mWriteEnableChannel);
	}
}

void NANDFlashAnalyzer::WorkerThread()
{
	Setup();

	for (;;)
	{
		/* 1. Check state: Reading, Writing, or UNKNOWN */

		/* 2. Get the current data (byte) */
		GetByte();
	}
}

void NANDFlashAnalyzer::Setup(void)
{
	mCLE = GetAnalyzerChannelData(mSettings->mCLEChannel);
	mALE = GetAnalyzerChannelData(mSettings->mALEChannel);
	mCE = GetAnalyzerChannelData(mSettings->mCEChannel);
	mReadEnable = GetAnalyzerChannelData(mSettings->mReadEnableChannel);
	mWriteEnable = GetAnalyzerChannelData(mSettings->mWriteEnableChannel);

    if (mSettings->mUsingIOChannels) {
        mData[0] = GetAnalyzerChannelData(mSettings->mIO0Channel);
        mData[1] = GetAnalyzerChannelData(mSettings->mIO1Channel);
        mData[2] = GetAnalyzerChannelData(mSettings->mIO2Channel);
        mData[3] = GetAnalyzerChannelData(mSettings->mIO3Channel);
        mData[4] = GetAnalyzerChannelData(mSettings->mIO4Channel);
        mData[5] = GetAnalyzerChannelData(mSettings->mIO5Channel);
        mData[6] = GetAnalyzerChannelData(mSettings->mIO6Channel);
        mData[7] = GetAnalyzerChannelData(mSettings->mIO7Channel);
    }
}

void NANDFlashAnalyzer::AdvanceToReadOrWriteEnableEdge(void)
{
	U64 current_sample = 0;
	U64 next_read_enable_sample = 0;
	U64 next_write_enable_sample = 0;
	U64 next_sample = 0;

    /* getting the next edge seems to block if there isn't another edge available */
    if (mMoreReadTransitions) {
        mMoreReadTransitions = mReadEnable->DoMoreTransitionsExistInCurrentData();
    }
    if (mMoreWriteTransitions) {
        mMoreWriteTransitions = mWriteEnable->DoMoreTransitionsExistInCurrentData();
    }

    /* we should allow the block to happen if there is currently no more data available */
    bool need_more_data = !mMoreReadTransitions && !mMoreWriteTransitions;

    /* lets mark this packet as then end as there is no more data right now */
    if (need_more_data) {
        FrameV2 frame_v2;
        mResults->AddFrameV2(frame_v2, "End", mReadEnable->GetSampleNumber(),
                             mReadEnable->GetSampleNumber() + 1);
	    mResults->CommitResults();
    }

	if (mReadEnable != NULL)
	{
        if (mMoreReadTransitions || need_more_data) {
		    next_read_enable_sample = mReadEnable->GetSampleOfNextEdge();
        }
	}

	if (mWriteEnable != NULL)
	{
        /* getting the next edge seems to block if there isn't another edge available */
        if (mMoreWriteTransitions) {
		    next_write_enable_sample = mWriteEnable->GetSampleOfNextEdge();
        }
	}

	/* Samples are time based so lowest value is next sample and make sure there is another sample available */
	if ((next_read_enable_sample <= next_write_enable_sample && mMoreReadTransitions) ||
        (mMoreReadTransitions && !mMoreWriteTransitions))
	{
		AdvanceToReadEnableHighEdge(); // Reads happen on the falling edge of the ReadEnable line

		/* NOTE: According to the datasheet, there is a read access time, tREA, ranging
		 * between 0 - 16ns. We'll try reading on the rising edge of the signal, which is usually
		 * between 20-30ns in order to ensure we make it past the read access time.
		 */
		next_sample = mReadEnable->GetSampleNumber();

		mDataIsOutput = true;
	}
	else if(mMoreWriteTransitions)
	{
		AdvanceToWriteEnableHighEdge(); // Writes happen on the rising edge of the WriteEnable line
		next_sample = mWriteEnable->GetSampleNumber();
		mDataIsOutput = false;
	}

	SynchronizeAllChannels(next_sample);
}

void NANDFlashAnalyzer::AdvanceToReadEnableHighEdge(void)
{
	if (mReadEnable != NULL)
	{
		mReadEnable->AdvanceToNextEdge();

		/* If we advance to a high edge, advance again to the next edge, which will be low */
		if (mReadEnable->GetBitState() != BIT_HIGH)
		{
			mReadEnable->AdvanceToNextEdge();
		}

		// check for valid read/write where both are not low
		U64 current_sample = mReadEnable->GetSampleNumber();
		mWriteEnable->AdvanceToAbsPosition(current_sample);

		// if (mWriteEnable->GetBitState() == BIT_HIGH)
		// {
		// 	mResults->AddMarker(mReadEnable->GetSampleNumber(), AnalyzerResults::UpArrow, mSettings->mReadEnableChannel);
		// 	mResults->CommitResults();
		// }
	}
}

void NANDFlashAnalyzer::AdvanceToWriteEnableHighEdge(void)
{
	if (mWriteEnable != NULL)
	{
		mWriteEnable->AdvanceToNextEdge();

		/* If we advance to a high edge, advance again to the next edge, which will be low */
		if (mWriteEnable->GetBitState() != BIT_HIGH)
		{
			mWriteEnable->AdvanceToNextEdge();
		}

		// check for valid read/write where both are not low
		U64 current_sample = mWriteEnable->GetSampleNumber();
		mReadEnable->AdvanceToAbsPosition(current_sample);

		if (mReadEnable->GetBitState() == BIT_HIGH)
		{
			mResults->AddMarker(mWriteEnable->GetSampleNumber(), AnalyzerResults::UpArrow, mSettings->mWriteEnableChannel);
			mResults->CommitResults();
		}
	}
}

void NANDFlashAnalyzer::SynchronizeAllChannels(U64 sample_number)
{
	mCLE->AdvanceToAbsPosition(sample_number);
	mALE->AdvanceToAbsPosition(sample_number);
	mCE->AdvanceToAbsPosition(sample_number);
	mReadEnable->AdvanceToAbsPosition(sample_number);
	mWriteEnable->AdvanceToAbsPosition(sample_number);

    if (mSettings->mUsingIOChannels) {
        mData[0]->AdvanceToAbsPosition(sample_number);
        mData[1]->AdvanceToAbsPosition(sample_number);
        mData[2]->AdvanceToAbsPosition(sample_number);
        mData[3]->AdvanceToAbsPosition(sample_number);
        mData[4]->AdvanceToAbsPosition(sample_number);
        mData[5]->AdvanceToAbsPosition(sample_number);
        mData[6]->AdvanceToAbsPosition(sample_number);
        mData[7]->AdvanceToAbsPosition(sample_number);
    }
}

void NANDFlashAnalyzer::GetByte(void)
{
	U8 data = 0;

	AdvanceToReadOrWriteEnableEdge();

	// Ignore anything where CE_N is high
	if (mCE->GetBitState() == BIT_HIGH) {
        return;
    }

	Frame frame;
    FrameV2 frame_v2;
    std::unordered_map<int, const char*> type_to_str = {
        {Read, "Read"},
        {Write, "Write"},
        {Command, "Command"},
        {Address, "Address"},
    };

	if (mDataIsOutput)
	{
		// Output

		// Check for undefined behavior first (both low)
		if (mWriteEnable->GetBitState() == BIT_LOW)
		{
			frame.mType = Undefined;
			mResults->AddFrame(frame);
			mResults->CommitResults();
			return;
		}

		// Get the data - 1 for any reads to account for RE rising edge
		if (mSettings->mUsingIOChannels)
		{
			for (U32 i = 0; i < 8; i++)
			{
				// Get previous sample of rising edge just for sanity's sake
				mData[i]->AdvanceToAbsPosition(mData[i]->GetSampleNumber() - 1);
				if (mData[i]->GetBitState() == BIT_HIGH)
				{
					data |= (1 << i);
				}
				mData[i]->AdvanceToAbsPosition(mData[i]->GetSampleNumber() + 1);
			}
		}
		else
		{
			data = 0;
		}
		
		U64 starting_sample = mReadEnable->GetSampleNumber();
		frame.mStartingSampleInclusive = starting_sample - 1;
		frame.mEndingSampleInclusive = starting_sample; // falling edge so place the bubble after
		mResults->AddMarker(starting_sample - 1, AnalyzerResults::Dot, mSettings->mReadEnableChannel);
		frame.mType = Read;
		frame.mFlags = 0;
	}
	else if (!mDataIsOutput)
	{
		/* Get the data */
		if (mSettings->mUsingIOChannels)
		{
			for (U32 i = 0; i < 8; i++)
			{
				if (mData[i]->GetBitState() == BIT_HIGH)
				{
					data |= (1 << i);
				}
			}
		}
		else
		{
			data = 0;
		}

		// Input
		U64 starting_sample = mWriteEnable->GetSampleNumber();
		frame.mStartingSampleInclusive = starting_sample - 1;
		frame.mEndingSampleInclusive = starting_sample; // rising edge so place the bubble before
		frame.mFlags = 0;

		// Check for undefined behavior first (both low)
		if (mReadEnable->GetBitState() == BIT_LOW)
		{
			frame.mType = Undefined;
			mResults->AddFrame(frame);
			mResults->CommitResults();
			return;
		}

		if (mCLE->GetBitState() == BIT_HIGH && mALE->GetBitState() == BIT_LOW)
		{
			// Command operation
			frame.mType = Command;
			mResults->AddMarker(starting_sample, AnalyzerResults::Dot, mSettings->mCLEChannel);
			mLastCommand = data;
		}
		else if (mALE->GetBitState() == BIT_HIGH && mCLE->GetBitState() == BIT_LOW)
		{
			// Address operation
			frame.mType = Address;
			mResults->AddMarker(starting_sample, AnalyzerResults::Dot, mSettings->mALEChannel);
		}
		else if (mALE->GetBitState() == BIT_LOW && mCLE->GetBitState() == BIT_LOW)
		{
			// Write operation
			frame.mType = Write;
            if (mSettings->mUsingIOChannels) {
                mResults->AddMarker(starting_sample, AnalyzerResults::Dot, mSettings->mIO0Channel);
                mResults->AddMarker(starting_sample, AnalyzerResults::Dot, mSettings->mIO1Channel);
                mResults->AddMarker(starting_sample, AnalyzerResults::Dot, mSettings->mIO2Channel);
                mResults->AddMarker(starting_sample, AnalyzerResults::Dot, mSettings->mIO3Channel);
                mResults->AddMarker(starting_sample, AnalyzerResults::Dot, mSettings->mIO4Channel);
                mResults->AddMarker(starting_sample, AnalyzerResults::Dot, mSettings->mIO5Channel);
                mResults->AddMarker(starting_sample, AnalyzerResults::Dot, mSettings->mIO6Channel);
                mResults->AddMarker(starting_sample, AnalyzerResults::Dot, mSettings->mIO7Channel);
            }
		}
	}

	frame.mData1 = data;
    frame_v2.AddInteger("Data", data);
    mResults->AddFrameV2(frame_v2, type_to_str[frame.mType], frame.mStartingSampleInclusive,
                         frame.mEndingSampleInclusive);
	mResults->AddFrame(frame);
	mResults->CommitResults();
}

bool NANDFlashAnalyzer::NeedsRerun()
{
	return false;
}

U32 NANDFlashAnalyzer::GenerateSimulationData(U64 minimum_sample_index, U32 device_sample_rate, SimulationChannelDescriptor **simulation_channels)
{
	if (mSimulationInitilized == false)
	{
		mSimulationDataGenerator.Initialize(GetSimulationSampleRate(), mSettings.get());
		mSimulationInitilized = true;
	}

	return mSimulationDataGenerator.GenerateSimulationData(minimum_sample_index, device_sample_rate, simulation_channels);
}

U32 NANDFlashAnalyzer::GetMinimumSampleRateHz()
{
	return 10000; // Unsure of the minimum; depends on the implementation; return the lowest rate.
}

const char *NANDFlashAnalyzer::GetAnalyzerName() const
{
	return "NAND Flash";
}

const char *GetAnalyzerName()
{
	return "NAND Flash";
}

Analyzer *CreateAnalyzer()
{
	return new NANDFlashAnalyzer();
}

void DestroyAnalyzer(Analyzer *analyzer)
{
	delete analyzer;
}
