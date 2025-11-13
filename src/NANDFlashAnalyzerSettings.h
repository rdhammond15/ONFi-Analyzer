#ifndef NANDFLASH_ANALYZER_SETTINGS
#define NANDFLASH_ANALYZER_SETTINGS

#include <AnalyzerSettings.h>
#include <AnalyzerTypes.h>

enum NANDFlashIOOperation
{
	NAND_READ,
	NAND_WRITE
};
enum NANDFlashCommand
{

};

class NANDFlashAnalyzerSettings : public AnalyzerSettings
{
public:
	NANDFlashAnalyzerSettings();
	virtual ~NANDFlashAnalyzerSettings();

	virtual bool SetSettingsFromInterfaces();
	void UpdateInterfacesFromSettings();
	virtual void LoadSettings(const char *settings);
	virtual const char *SaveSettings();

	bool mUsingIOChannels;
	Channel mIO0Channel;
	Channel mIO1Channel;
	Channel mIO2Channel;
	Channel mIO3Channel;
	Channel mIO4Channel;
	Channel mIO5Channel;
	Channel mIO6Channel;
	Channel mIO7Channel;
	Channel mWriteEnableChannel;
	Channel mReadEnableChannel;
	Channel mCLEChannel;
	Channel mALEChannel;
	Channel mCEChannel;

protected:
	std::unique_ptr<AnalyzerSettingInterfaceChannel> mIO0Interface;
	std::unique_ptr<AnalyzerSettingInterfaceChannel> mIO1Interface;
	std::unique_ptr<AnalyzerSettingInterfaceChannel> mIO2Interface;
	std::unique_ptr<AnalyzerSettingInterfaceChannel> mIO3Interface;
	std::unique_ptr<AnalyzerSettingInterfaceChannel> mIO4Interface;
	std::unique_ptr<AnalyzerSettingInterfaceChannel> mIO5Interface;
	std::unique_ptr<AnalyzerSettingInterfaceChannel> mIO6Interface;
	std::unique_ptr<AnalyzerSettingInterfaceChannel> mIO7Interface;
	std::unique_ptr<AnalyzerSettingInterfaceChannel> mWriteEnableInterface;
	std::unique_ptr<AnalyzerSettingInterfaceChannel> mReadEnableInterface;
	std::unique_ptr<AnalyzerSettingInterfaceChannel> mCLEInterface;
	std::unique_ptr<AnalyzerSettingInterfaceChannel> mALEInterface;
	std::unique_ptr<AnalyzerSettingInterfaceChannel> mCEInterface;
};

#endif // NANDFLASH_ANALYZER_SETTINGS
