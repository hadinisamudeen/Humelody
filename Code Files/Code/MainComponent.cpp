/*
  ==============================================================================

    This file was auto-generated!

  ==============================================================================
*/

#include "MainComponent.h"
//#include "AudioRecorder.h"
#include "Yin.h"
#include<cmath>
//==============================================================================
MainComponent::MainComponent() : State(tStopped), startTime(Time::getMillisecondCounterHiRes() * 0.001)
{
	//setting the size of the window
	setSize(600, 400);
	
	
	
	addAndMakeVisible(explainLabel);
	explainLabel.setText("1. Click 'Record' to record, OR 2. click Import to directly import a file.",NotificationType::dontSendNotification);
	
	addAndMakeVisible(&Record);
	Record.setButtonText("Record");
	Record.setColour(TextButton::buttonColourId, Colours::darkred);

	addAndMakeVisible(&StopRecord);
	StopRecord.setButtonText("STOP REC");
	StopRecord.setColour(TextButton::buttonColourId, Colours::antiquewhite);
	StopRecord.setEnabled(false);

	addAndMakeVisible(Import);
	Import.setButtonText("Import");
	Import.setColour(TextButton::buttonColourId, Colours::black);


	addAndMakeVisible(Play);
	Play.setButtonText("Play");
	Play.setEnabled(false);

	addAndMakeVisible(&Stop);
	Stop.setButtonText("Stop");
	Stop.setEnabled(false);

	
	addAndMakeVisible(&findPitch);
	findPitch.setButtonText("HUMELODY!");
	findPitch.setColour(TextButton::buttonColourId,Colours::lightseagreen);
	findPitch.setEnabled(false);

	addAndMakeVisible(&Reset);
	Reset.setButtonText("r e     s e t !");
	Reset.setColour(TextButton::buttonColourId, Colours::darkcyan);

	importFormatManager.registerBasicFormats();
	transport.addChangeListener(this);

	
	backgroundThread.startThread();

	//setting functions for the buttons
	findPitch.onClick = [this] {findPitchButtonClicked();};
	Import.onClick = [this] {importButtonClicked();};
	Play.onClick = [this] {playButtonClicked();};
	Stop.onClick = [this] {stopButtonClicked();};
	Record.onClick = [this] {recordPressed();};
	StopRecord.onClick = [this] {stopPressed();};
	Reset.onClick = [this] {resetPressed();};
	toBeImported.setSize(2, 100000);
	manager.registerBasicFormats();
	
	
    // Some platforms require permissions to open input channels so request that here
    if (RuntimePermissions::isRequired (RuntimePermissions::recordAudio)
        && ! RuntimePermissions::isGranted (RuntimePermissions::recordAudio))
    {
        RuntimePermissions::request (RuntimePermissions::recordAudio,
       [&] (bool granted) { if (granted)  setAudioChannels (2, 2); });
    
		
	}
    else
    {
        // Specify the number of input and output channels that we want to open
        setAudioChannels (2, 2);
    }

	
	
}

MainComponent::~MainComponent()
{
    // This shuts down the audio device and clears the audio source.
	stop();
    shutdownAudio();
}

//==============================================================================

void MainComponent::resetPressed()
{
	Record.setEnabled(true);
	StopRecord.setEnabled(false);
	Import.setEnabled(true);
	Play.setEnabled(false);
	Stop.setEnabled(false);
	findPitch.setEnabled(false);
	explainLabel.setText("1. Click 'Record' to record, OR 2. click Import to directly import a file.", NotificationType::dontSendNotification);
	mRecordReady = -1;
	transportFlag = -1;
}

void MainComponent::stop()
{
	// First, clear this pointer to stop the audio callback from using our writer object..
	{

		DBG("RECORDER.stop() CALLED");
		const ScopedLock sl(writerLock);
		activeWriter = nullptr;
	}

	// Now we can delete the writer object. It's done in this order because the deletion could
	// take a little time while remaining data gets flushed to disk, so it's best to avoid blocking
	// the audio callback while this happens.
	threadedWriter.reset();
}


void MainComponent::stopPressed()
{
	
	DBG("STOP PRESSED!");
	mBeingRecorded = 0;
	mRecordReady = 0;
	explainLabel.setText("File saved in Documents folder! Click on 'Import' to import the file.", NotificationType::dontSendNotification);
	stop();

	lastRecording = File();

	StopRecord.setEnabled(false);
	Record.setEnabled(true);
	Import.setEnabled(true);
}


void MainComponent::startRecording(const File& file)
{
	stop();

	StopRecord.setEnabled(true);
	Record.setEnabled(false);
	Import.setEnabled(false);


	//DBG("RECORDER.startRecording() CALLED");
	if (mSampleRate > 0)
	{
		//DBG("RATE");
		// Create an OutputStream to write to our destination file...
		file.deleteFile();

		if (auto fileStream = std::unique_ptr<FileOutputStream>(file.createOutputStream()))
		{
			DBG("FILE");
			// Now create a WAV writer object that writes to our output stream...
			WavAudioFormat wavFormat;

			if (auto writer = wavFormat.createWriterFor(fileStream.get(), mSampleRate, 1, 16, {}, 0))
			{

				fileStream.release(); // (passes responsibility for deleting the stream to the writer object that is now using it)

									  // Now we'll create one of these helper objects which will act as a FIFO buffer, and will
									  // write the data to disk on our background thread.
				threadedWriter.reset(new AudioFormatWriter::ThreadedWriter(writer, backgroundThread, 32768));
				//DBG("resetted");
				// Reset our recording thumbnail
				//thumbnail.reset(writer->getNumChannels(), writer->getSampleRate());
				//nextSampleNum = 0;

				// And now, swap over our active writer pointer so that the audio callback will start using it..

				const ScopedLock sl(writerLock);
				activeWriter = threadedWriter.get();

				DBG("RECORD READY");
			}
		}
	}
	DBG("ALL SET");
	mRecordReady = 1;
	transportFlag = 0;
	explainLabel.setText("Recording...Click 'STOP REC' when done.", NotificationType::dontSendNotification);
}




void MainComponent::recordPressed()

{
	mBeingRecorded = 1;
	auto parentDir = File::getSpecialLocation(File::userDocumentsDirectory);
	lastRecording = parentDir.getNonexistentChildFile("HUMELODY Recording", ".wav");

	startRecording(lastRecording);

	
}





void MainComponent::prepareToPlay (int samplesPerBlockExpected, double sampleRate)
{

	transport.prepareToPlay(samplesPerBlockExpected, sampleRate);
	
    // This function will be called when the audio device is started, or when
    // its settings (i.e. sample rate, block size, etc) are changed.

    // this function is used to initialise any resources you might need,
    // it will be called on the audio thread, not the GUI thread.

    
	String message;
	message << "Preparing to play audio..." << newLine;
	message << " samplesPerBlockExpected = " << samplesPerBlockExpected << newLine;
	message << " sampleRate = " << sampleRate;
	Logger::getCurrentLogger()->writeToLog(message);

	const int numInputChannels = 2;

	//buffer for 15 seconds
	mSampleRate = sampleRate;
	const int numSamples = sampleRate * 15;
	mRecBuffer.setSize(numInputChannels, numSamples);
}



void MainComponent::getNextAudioBlock(const AudioSourceChannelInfo& bufferToFill)
{

	if ((mRecordReady == 1) && (transportFlag == 0))
	{
		//setAudioChannels(2, 2);
		//bufferToFill.clearActiveBufferRegion();
		
		auto* device = deviceManager.getCurrentAudioDevice();
		auto activeInputChannels = device->getActiveInputChannels();
		auto activeOutputChannels = device->getActiveOutputChannels();
		auto maxInputChannels = activeInputChannels.getHighestBit() + 1;
		auto maxOutputChannels = activeOutputChannels.getHighestBit() + 1;
		//device->;
		//	DBG(maxInputChannels);
		const float** channelpointers;

		channelpointers[0] = bufferToFill.buffer->getReadPointer(0, bufferToFill.startSample);
		channelpointers[1] = bufferToFill.buffer->getReadPointer(1, bufferToFill.startSample);
		DBG(bufferToFill.numSamples);
		activeWriter.load()->write(channelpointers, bufferToFill.numSamples);
		//bufferToFill.startSample = bufferToFill.startSample + bufferToFill.numSamples;

	}
	else if ((mRecordReady == 0) && (transportFlag == 1))
	{
	
		//   bufferToFill.clearActiveBufferRegion();
			transportCallBlock(bufferToFill);
	}


}


void MainComponent::transportCallBlock(const AudioSourceChannelInfo& bufferToFill)
{
	transport.getNextAudioBlock(bufferToFill);
}


void MainComponent::importButtonClicked()
{

	FileChooser chooser("Choose a WAV/MP3 file...", {}, "*.wav");
	
	if (chooser.browseForFileToOpen())
	{
		pointerboi = 0;
		yindoneboi = 0;
		File importedFile;
		//'store' the chosen file
		importedFile = chooser.getResult();

		//read the file
		reader = importFormatManager.createReaderFor(importedFile);

		//ready up the file
		if (reader != nullptr)
		{
			std::unique_ptr<AudioFormatReaderSource> tempSource(new AudioFormatReaderSource(reader, true));

			//setting the source of the transport file to play the audio
			transport.setSource(tempSource.get());
			transportStateChanged(tStopped);
			playSource.reset(tempSource.release());
			DBG("READER CHANNELZ");
			DBG(reader->numChannels);
			
		}

			//	yin.printYin();
			
				//var(pitchR)::to
	}
		
	Play.setEnabled(true);
	Stop.setEnabled(true);
	Record.setEnabled(false);
	findPitch.setEnabled(true);
	explainLabel.setText("CLick Play and Stop to listen to the file, and click on 'HUMELODY!' to do its magic!!", NotificationType::dontSendNotification);
	transportFlag = 1;
	mRecordReady = 0;
}
		
	


void MainComponent::playButtonClicked()
{
	transportStateChanged(tStarting);
}


void MainComponent::stopButtonClicked()
{
	transportStateChanged(tStopping);
}



void MainComponent::findPitchButtonClicked()
{
	if (newMidiFile.getNumTracks() != 0)
	{
		newMidiFile.clear();
		messageSequence.clear();
	}
	AudioBuffer<float> importedBuffer(reader->numChannels, reader->lengthInSamples);
	reader->read(&importedBuffer, 0, reader->lengthInSamples, 0, true, true);

	std::vector<float> chanLeft(reader->lengthInSamples);
	std::vector<float> chanRight(reader->lengthInSamples);
	std::vector<float> chanMean(reader->lengthInSamples);

	int i, j;
	counterboi = 0;
	//explainLabel.setText("HUMELODY working...", NotificationType::dontSendNotification);
	DBG(reader->lengthInSamples);

	for (int channel = 0;channel < reader->numChannels;channel++)
	{

		//insertion of the values

		if (channel == 0)
		{
			for (i = 0;i < reader->lengthInSamples; i++)
			{
				chanLeft[i] = (importedBuffer.getSample(channel, i));
			}
		}

		else if (channel == 1)
		{
			for (j = 0;j < reader->lengthInSamples; j++)
			{
				chanRight[j] = (importedBuffer.getSample(channel, j));
			}
		}


	}

	// WHEN BOTH THE ARRAYS ARE FILLED
	if ((j == reader->lengthInSamples) && (i == reader->lengthInSamples) && (reader->numChannels == 2))

	{
		for (int x = 0;x < reader->lengthInSamples; x++)
		{
			chanMean[x] = ((chanLeft.at(x) + chanRight.at(x)) / 2);
		}


	}

	else if ((i == reader->lengthInSamples) && (reader->numChannels == 1))

	{


		for (int x = 0;x < reader->lengthInSamples; x++)
		{
			chanMean[x] = (chanLeft.at(x));
		
		}


	}
	//setting a bufferize for the minibuffer to get the fundamental frequency
	//it was initially found using this method:
	
	/*
		buffersize = 100;
		pitchR = -1;
		while (pitchR == -1)
		{
			pitchR = yin.getPitch(pitchFinder(bufferSize);
		}

	*/
	buffersize = 3000;

	//PITCH VALUE
	pitchR = 0;
	//a 'pointer' to the index of the actual values of the buffer
	int pitcherSample = 0;

	int counterr = 0;

	DBG("BufferSize = ");
	DBG(buffersize);
	DBG(chanMean.size());


	//to set time stamps, the previous and current pitchvals are stores to compare if the same pitch is being read or not.
	int midipitchval = -1;
	int previousmidipitchval;

	//Semitone value
	const float semival = 1.05946;

	//Sample number
	int pitchSampleNumber = 0;

	//flag for the first message entered ever
	int firstvalue = 0;

	
	auto tempoMessage = MidiMessage::tempoMetaEvent(60000000/130);
	auto timeSigMessage = MidiMessage::timeSignatureMetaEvent(4,4);

	tempoMessage.setTimeStamp(0);
	timeSigMessage.setTimeStamp(0);

	messageSequence.addEvent(tempoMessage);
	messageSequence.addEvent(timeSigMessage);

	
	do
	{

		previousmidipitchval = midipitchval;
		//DBG(semival);
		//finding the mean value of toneHertz and highToneHertz to find out how close the yin pitch value is
		float meanoftones;

		//different Pitch values in Hertz to find out how similar the pitch detected by YIN is 
		float toneHertz;
		float highToneHertz;

		//a variable used as a flag to see if the pitch returned is an empty val or nah
		int isnoteMinusOne = 0;


		std::vector<float> pitchFinder(buffersize);
		for (int i = 0;i < buffersize;i++)
		{
			pitchFinder[i] = (chanMean.at(pitcherSample));
			pitcherSample++;
		}


		//initialisation of the yin buffer and getting the pitch value in Hz
		yin.initialize(mSampleRate, buffersize);
		pitchR = yin.getPitch(pitchFinder);
	


		DBG("/////////////////////////////////");
		//toneHertz = 8.18;
		highToneHertz = 8.18;

		//GIVING THE MIDI NOTE NUMBER
		for (int i = 0;i< 127; i++)
		{
			toneHertz = highToneHertz;
			highToneHertz = toneHertz * semival;
			meanoftones = (highToneHertz + toneHertz) / 2;
			if (pitchR == -1)
			{
				midipitchval = -1;
				break;
			}
			else if ((pitchR < meanoftones) && (pitchR >= toneHertz))
			{
				midipitchval = i;
			

				break;
			}
			else if ((pitchR >= meanoftones) && (pitchR < highToneHertz))
			{
				midipitchval = i + 1;
				
				break;
			}

		}


		DBG("PITCH VALUE ");
		DBG(midipitchval);

		//THE TIMESTAMP IN MILLISECONDS
		float stampInMilliSecs = (float)(pitchSampleNumber * 1000.0 * 3252.0) / (float)mSampleRate;

		//Pulses Per Quarter Note is set to 960 here
		float PPQ = 960.0;


		//probably cause of the recording having a delay in the start of the melody
		//OR because there were no notes played before,i.e, the note played before was notedOff
		//checking if the midipitchval has a value and previous doesnt

		if (previousmidipitchval == -1 && midipitchval != -1 && midipitchval != 0)
		{

			auto message = MidiMessage::noteOn(1, midipitchval, (uint8)100);
			message.setTimeStamp((float)(((130.0 * PPQ) / 60000.0)* stampInMilliSecs));
			messageSequence.addEvent(message);
		}

		//if there was a note playing before ,it cuts off here
		else if (midipitchval == -1 && previousmidipitchval != -1)
		{
			auto message = MidiMessage::noteOff(1, previousmidipitchval);
			message.setTimeStamp((float)(((130.0 * PPQ) / 60000.0)* stampInMilliSecs));
			messageSequence.addEvent(message);
		}



		
		//Else if there is a sudden change in note
		else if ((midipitchval != previousmidipitchval) && previousmidipitchval != -1 && previousmidipitchval != 0 && midipitchval != -1 && midipitchval != 0)

		{

			auto message1 = MidiMessage::noteOff(1, previousmidipitchval);
			auto message2 = MidiMessage::noteOn(1, midipitchval, (uint8)100);

			//CONVERTING MILLISECONDS TO TICKS
			//NOTE: x milliseconds = ((BPM * PPQ)/60000) * x ticks

			message1.setTimeStamp((float)(((130.0 * PPQ) / 60000.0)* stampInMilliSecs));
			message2.setTimeStamp((float)(((130.0 * PPQ) / 60000.0)* stampInMilliSecs));
			//(stampInMilliSecs * 120.0 * PPQ) / 60000.0)

			messageSequence.addEvent(message1);
			messageSequence.addEvent(message2);
		}

		
		pitchFinder.clear();
		counterr++;

		//INCREMENTING THE SAMPLE NUMBER
		pitchSampleNumber++;

	} while ((pitcherSample + buffersize < chanMean.size()));

	newMidiFile.setTicksPerQuarterNote(960);
	newMidiFile.addTrack(messageSequence);
	auto finalParentDir = File::getSpecialLocation(File::userDocumentsDirectory);
	File file = finalParentDir.getNonexistentChildFile("HUMELODMIDI", ".mid");
	FileOutputStream fileStream(file);
	fileStream.setPosition(0);
	newMidiFile.writeTo(fileStream);

	DBG("Number of tracks!");
	DBG(newMidiFile.getNumTracks());

	explainLabel.setText("MIDI CREATED AS HUMELODY! CHECK YOUR DOCUMENTS FOLDER!", NotificationType::dontSendNotification);
	DBG("MESSAGE SEQUENCE TOTAL EVENTS");
	DBG(messageSequence.getNumEvents());
	messageSequence.getEventPointer(0)->message.getNoteNumber();


}




void MainComponent::transportStateChanged(transportState newState)
{
	if (newState != State)
	{
		State = newState;
		
		switch (State) {
		case tStopped: transport.setPosition(0.0);
			break;
		case tStarting: 
			//Stop.setEnabled(true)
			transport.start();
			DBG("Playing");
			break;
		case tStopping: 
			//Play.setEnabled(true);
			transport.stop();
			DBG("Stopping");
			break;
		case tPlaying: 
			Play.setEnabled(true);
			break;
		}

	}

}

void MainComponent::changeListenerCallback(ChangeBroadcaster *source)
{
	if (source == &transport)
	{
		if (transport.isPlaying())
		{
			transportStateChanged(tPlaying);
		}
		else
		{
			transportStateChanged(tStopped);
		}
	}
}




void MainComponent::releaseResources()
{
    // This will be called when the audio device stops, or when it is being
    // restarted due to a setting change.

    // For more details, see the help for AudioProcessor::releaseResources()
}

//==============================================================================
void MainComponent::paint (Graphics& g)
{
	g.fillAll(Colours::black);
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll (getLookAndFeel().findColour (ResizableWindow::backgroundColourId));
	//g.drawFittedText("Humelody is here!", getLocalBounds(), Justification::centred, 1);
	Path p;
	g.setColour(Colours::black);
	g.drawLine(0,100,getWidth(),100);
    // You can add your drawing code here!
}

void MainComponent::resized()
{
	//Record.set
	Record.setBounds(10, 25, 100, 50);
	StopRecord.setBounds(115, 25, 100, 50);
	
	Import.setBounds(220, 25, 150, 50);

	Play.setBounds(375, 25, 100, 50);
	Stop.setBounds(480, 25, 100, 50);

	explainLabel.setBounds(200, 200, 250, 50);

	findPitch.setBounds(((getWidth() / 4)), getHeight() - 100 , (getWidth() / 2), 50);
	Reset.setBounds(0, getHeight() - 45, 55, 45);
}


