/*
  ==============================================================================
  MAINCOMPONENT.H
  ==============================================================================
*/

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include "Yin.h"
//#include "AudioRecorder.h"
//#include "AudioLiveScrollingDisplay.h"
//==============================================================================
/*
    This component lives inside our window, and this is where you should put all
    your controls and content.
*/
class MainComponent   : public AudioAppComponent,
						public ChangeListener
				
{
public:
    //==============================================================================
    MainComponent();
    ~MainComponent();

    //==============================================================================
    void prepareToPlay (int samplesPerBlockExpected, double sampleRate) override;
    void getNextAudioBlock (const AudioSourceChannelInfo& bufferToFill) override;
    void releaseResources() override;

    //==============================================================================
    void paint (Graphics& g) override;
    void resized() override;

private:
	//AudioDeviceManager adm;
	//std::unique_ptr<AudioDeviceSelectorComponent> audioSettings;


	enum transportState
	{
		tStopped,
		tStarting,
		tStopping,
		tPlaying
	};

	transportState State;

	void importButtonClicked();
	void playButtonClicked();
	void resetPressed();
	void findPitchButtonClicked();
	void stopButtonClicked();
	void transportStateChanged(transportState newState);
	void changeListenerCallback(ChangeBroadcaster *source) override;
	


	void transportCallBlock(const AudioSourceChannelInfo& bufferToFill);

	
//FOR THE RECORDING
	void startRecording(const File& file);
	void stop();
	void recordPressed();
	void stopPressed();
	
	TimeSliceThread backgroundThread{ "Audio Recorder Thread" }; // the thread that will write our audio data to disk
	std::unique_ptr<AudioFormatWriter::ThreadedWriter> threadedWriter; // the FIFO used to buffer the incoming data
	
	//when record button is pressed, this turns 1
	int mBeingRecorded = 0;

	//when startrecording is set, this turns 1
	int mRecordReady = -1;

	int testFlag = 2;
	CriticalSection writerLock;
	std::atomic<AudioFormatWriter::ThreadedWriter*> activeWriter{ nullptr };
	File lastRecording;

//FOR THE REST OF THE AUDIO
	AudioFormatManager importFormatManager;
	AudioDeviceManager audioDeviceManager;
	std::unique_ptr<AudioFormatReaderSource> playSource;
	AudioTransportSource transport;

	TextButton Record;
	TextButton StopRecord;
	TextButton Stop;
	TextButton Play;
	TextButton Import;
	TextButton findPitch;
	TextButton Reset;

	Yin yin;
	AudioFormatManager manager;
	//float 

	//RECORDING VALS
	int mSampleRate{ 44100 };
	AudioBuffer<float> mRecBuffer;

	AudioBuffer<float> toBeImported;

	float* bufferToBeSent;
	//float array[];

	int counterboi;
	int pointerboi;
	int yindoneboi;

	int buffersize;

	float pitchR;
	
	double startTime;

	Label explainLabel;
	//float* importBufferData;

	int transportFlag{ -1 };

	MidiFile newMidiFile;
	AudioFormatReader *reader;
	MidiMessageSequence messageSequence;
	//==============================================================================
    // Your private member variables go here...


    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};
