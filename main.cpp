#include "stdafx.h"
#include <iostream>
#include <string>
#include <sstream>
#include <fstream>
#include "AgMD1Fundamental.h"

#include <cmath>

using namespace std;


/*
Big TODO:
- make library with functions initialization and closing;
- make library with functions configuration;
- add readable error messages.
*/



int _tmain(int argc, _TCHAR* argv[]) {
	cout << "Fuck you!" << endl; // Just greeting

	// TODO: make a log-file for errors
	// TODO: make a special function for this shit

	// If you don't know which and/or how many instruments
	// are present on the machine, use this code fragment
	ViSession instrumentID[10];
	ViInt32 nbrInstruments;
	ViStatus status;
	ViString options = "";
	status = Acqrs_getNbrInstruments(&nbrInstruments);
	// TODO: add exit if number of instruments is 0
	cout << "The number of available instruments is " << nbrInstruments << endl;


	// TODO: make a special function for this shit
	// Initialize the instruments
	for (ViInt32 devIndex = 0; devIndex < nbrInstruments; devIndex++) {
		ViInt32 devTypeP;
		ViStatus status = Acqrs_getDevTypeByIndex(devIndex, &devTypeP); // get type of instrument

		if (devTypeP == 1) {
			cout << "The number " << devIndex << " is Digitizer" << endl;
		}
		else if (devTypeP == 2) {
			cout << "The number " << devIndex << " is RC2xx Generator" << endl;
		}
		else if (devTypeP == 3) {
			cout << "The number " << devIndex << " is TC Time-to-Digital Converter" << endl;
		}

		//ViRsrc resourceName = "PCI::INSTR0"; // TODO: try to fix it later
		char resourceName[20] = "20";
		sprintf_s(resourceName, "PCI::INSTR%d", devIndex);
		status = Acqrs_InitWithOptions(resourceName, VI_FALSE, VI_FALSE, options, &(instrumentID[devIndex]));

		if (status == 0) {
			cout << "Instrument number " << devIndex << " successfully initialized!" << endl;
			cout << "ID: " << instrumentID[devIndex] << endl;
		}
		else {
			cout << "Something go wrong... Error code: " << status << endl;
		}
	}


	/* ============================================================================================================== */
	// Infinity loop
	int workAgain = 1;

	while (workAgain == 1) {
		// TODO: make a special function for this shit
		// Configure equipment
		// Configuration values:
		ifstream configFile("C:/Users/SE1/Desktop/data_acquisition/config.txt");
		string configValue;
		long counter = 1;
		double parameters[11];

		if (configFile.is_open()) {
			cout << "Configuration file opened successfull!" << endl;
			while (getline(configFile, configValue)) {
				if (counter % 2 == 0) {
					parameters[counter / 2 - 1] = stod(configValue);
				}

				counter++;
			}

			configFile.close();
		}
		else {
			cout << "Problems with configuration file..." << endl;
		}

		// TODO: comment variables with documentation
		double sampInterval = parameters[0] * 1e-9; // Calculate in seconds
		double delayTime = parameters[1] * 1e-9;; // Calculate in seconds
		long channel = (long)parameters[2];
		long coupling = (long)parameters[3];
		long bandwidth = (long)parameters[4];
		double fullScale = parameters[5] / 1000; // Calculate in Volts
		double offset = parameters[6];
		long trigCoupling = (long)parameters[7]; // Trigger  coupling  is  used  to  select  the  coupling  mode  applied  to  the  input  of  the  trigger  circuitry
		long trigSlope = (long)parameters[8]; // The trigger slope defines which one of the two possible transitions will be used to initiate the trigger when it passes through the specified trigger level
		// Positive slope (0) indicates that the signal is transitioning from a lower voltage to a higher. Negative slope (1) indicates the signal is transitioning from a higher to a lower voltage.
		// double trigLevel = parameters[9] / parameters[5] * 100; // Calculate in percents of vertical Full Scale
		double trigLevel = parameters[9] / 1000; // Calculate in Volts
		cout << "Enter triglevel (in mV): ";
		cin >> trigLevel;
		trigLevel = trigLevel / 1000;
		cout << "Entered triglevel is " << trigLevel << endl;
		long trigChannel = (long)parameters[10]; // Channel for triggering (for external need to be -1)


		long nbrSamples, nbrSegments;
		cout << "Enter number of samples: ";
		cin >> nbrSamples;
		cout << "Enter number of segments: ";
		cin >> nbrSegments;

		/* ================================================================================================== */

		int infLoopDataAcquire = 1;
		nbrSegments = 10;
		double means[10];
		double variances[10];

		for (long i = 0; i < nbrInstruments; i++) {
			AcqrsD1_configHorizontal(instrumentID[i], sampInterval, delayTime);
			AcqrsD1_configMemory(instrumentID[i], nbrSamples, nbrSegments);
			AcqrsD1_configVertical(instrumentID[i], channel, fullScale, offset, coupling, bandwidth);
			AcqrsD1_configTrigClass(instrumentID[i], 0, 0x80000000, 0, 0, 0.0, 0.0);
			AcqrsD1_configTrigSource(instrumentID[i], trigChannel, trigCoupling, trigSlope, trigLevel, 0.0);
		}
		cout << "Equipment configurated successfully!" << endl;

		while (infLoopDataAcquire == 1) {
			long timeOut = 2000;

			for (long i = 0; i < nbrInstruments; i++) {
				AcqrsD1_acquire(instrumentID[i]);
				status = AcqrsD1_waitForEndOfAcquisition(instrumentID[i], timeOut);
				cout << "Acquisition status is: " << status << endl;
			}

			if (status == 0) {
				// Definition of the read parameters for raw ADC readout
				long currentSegmentPad, nbrSamplesNom, nbrSegmentsNom;
				AqReadParameters readPar;
				readPar.dataType = ReadInt8; // 8bit, raw ADC values data type
				readPar.readMode = 1; // Multi-segment read mode
				readPar.firstSegment = 0;
				readPar.nbrSegments = nbrSegments;
				readPar.firstSampleInSeg = 0;
				readPar.nbrSamplesInSeg = nbrSamples;
				readPar.segmentOffset = nbrSamples;
				readPar.segDescArraySize = sizeof(AqSegmentDescriptor) * nbrSegments;

				readPar.flags = 0;
				readPar.reserved = 0;
				readPar.reserved2 = 0;
				readPar.reserved3 = 0;


				// Let's readout data
				for (long i = 0; i < nbrInstruments; i++) {
					Acqrs_getInstrumentInfo(instrumentID[i], "TbSegmentPad", &currentSegmentPad);
					status = AcqrsD1_getMemory(instrumentID[i], &nbrSamplesNom, &nbrSegmentsNom);
					cout << "Getting memory status is: " << status << endl;
					readPar.dataArraySize = (nbrSamplesNom + currentSegmentPad) * (nbrSegments + 1); // Array size in bytes
				}

				// Read the channel 1 waveform as raw ADC values
				AqDataDescriptor dataDesc;
				AqSegmentDescriptor *segDesc = new AqSegmentDescriptor[nbrSegments];
				ViInt8 * adcArrayP = new ViInt8[readPar.dataArraySize];

				for (long i = 0; i < nbrInstruments; i++) {
					status = AcqrsD1_readData(instrumentID[i], channel, &readPar, adcArrayP, &dataDesc, segDesc);
					cout << "Reading data status is " << status << endl;
				}

				// Let's calculate mean and variance
				double mean = 0;
				double variance = 0;

				/*for (ViInt32 i = 0; i < dataDesc.returnedSamplesPerSeg * dataDesc.returnedSegments; i++) {
					mean += int(adcArrayP[i]) * dataDesc.vGain - dataDesc.vOffset;
				}

				mean = mean / (dataDesc.returnedSamplesPerSeg * dataDesc.returnedSegments);

				for (ViInt32 i = 0; i < dataDesc.returnedSamplesPerSeg * dataDesc.returnedSegments; i++) {
					variance += pow(int(adcArrayP[i]) * dataDesc.vGain - dataDesc.vOffset - mean, 2);
				}

				variance = variance / (dataDesc.returnedSamplesPerSeg * dataDesc.returnedSegments);

				cout << "Mean: " << mean << endl << "Variance: " << variance << endl;*/

				for (int i = 0; i < nbrSegments; i++) {
					for (int j = nbrSamples * i; j < nbrSamples * (i + 1); j++) {
						mean += int(adcArrayP[j]) * dataDesc.vGain - dataDesc.vOffset;
					}

					mean = mean / (dataDesc.returnedSamplesPerSeg * dataDesc.returnedSegments);
					means[i] = mean;
					mean = 0;
				}

				for (int i = 0; i < nbrSegments; i++) {
					for (int j = nbrSamples * i; j < nbrSamples * (i + 1); j++) {
						variance += pow(int(adcArrayP[j]) * dataDesc.vGain - dataDesc.vOffset - means[i], 2);
					}

					variance = variance / (dataDesc.returnedSamplesPerSeg * dataDesc.returnedSegments);
					variances[i] = variance;
					variance = 0;
				}

				for (int i = 0; i < nbrSegments; i++) {
					cout << variances[i] << " ";
				}

				cout << endl;

				delete[] adcArrayP;
			}
		}

		/* ================================================================================================== */

		// Let's the fuck up
		// TODO: cover this functions in try-catch
		/*for (long i = 0; i < nbrInstruments; i++) {
			AcqrsD1_configHorizontal(instrumentID[i], sampInterval, delayTime);
			AcqrsD1_configMemory(instrumentID[i], nbrSamples, nbrSegments);
			AcqrsD1_configVertical(instrumentID[i], channel, fullScale, offset, coupling, bandwidth);
			AcqrsD1_configTrigClass(instrumentID[i], 0, 0x80000000, 0, 0, 0.0, 0.0);
			AcqrsD1_configTrigSource(instrumentID[i], trigChannel, trigCoupling, trigSlope, trigLevel, 0.0);
		}
		cout << "Equipment configurated successfully!" << endl;


		// Let's get data
		long timeOut = 2000;

		for (long i = 0; i < nbrInstruments; i++) {
			AcqrsD1_acquire(instrumentID[i]);
			status = AcqrsD1_waitForEndOfAcquisition(instrumentID[i], timeOut);
			cout << "Acquisition status is: " << status << endl;
		}

		if (status == 0) {
			// Definition of the read parameters for raw ADC readout
			long currentSegmentPad, nbrSamplesNom, nbrSegmentsNom;
			AqReadParameters readPar;
			readPar.dataType = ReadInt8; // 8bit, raw ADC values data type
			readPar.readMode = 1; // Multi-segment read mode
			readPar.firstSegment = 0;
			readPar.nbrSegments = nbrSegments;
			readPar.firstSampleInSeg = 0;
			readPar.nbrSamplesInSeg = nbrSamples;
			readPar.segmentOffset = nbrSamples;
			readPar.segDescArraySize = sizeof(AqSegmentDescriptor) * nbrSegments;

			readPar.flags = 0;
			readPar.reserved = 0;
			readPar.reserved2 = 0;
			readPar.reserved3 = 0;


			// Let's readout data
			for (long i = 0; i < nbrInstruments; i++) {
				Acqrs_getInstrumentInfo(instrumentID[i], "TbSegmentPad", &currentSegmentPad);
				status = AcqrsD1_getMemory(instrumentID[i], &nbrSamplesNom, &nbrSegmentsNom);
				cout << "Getting memory status is: " << status << endl;
				readPar.dataArraySize = (nbrSamplesNom + currentSegmentPad) * (nbrSegments + 1); // Array size in bytes
			}

			// Read the channel 1 waveform as raw ADC values
			AqDataDescriptor dataDesc;
			AqSegmentDescriptor *segDesc = new AqSegmentDescriptor[nbrSegments];
			ViInt8 * adcArrayP = new ViInt8[readPar.dataArraySize];

			for (long i = 0; i < nbrInstruments; i++) {
				status = AcqrsD1_readData(instrumentID[i], channel, &readPar, adcArrayP, &dataDesc, segDesc);
				cout << "Reading data status is " << status << endl;
			}


			// Write the waveform into a file
			// TODO: add date-mark in name of file: "acqiris-data-xx-xx-xxxx.data"
			ofstream outFile("C:/Users/SE1/Desktop/data_acquisition/acqiris.data");
			outFile << "# Agilent Acqiris Waveform Channel 1" << endl;
			outFile << "# Samples acquired: " << dataDesc.returnedSamplesPerSeg * dataDesc.returnedSegments << endl;
			outFile << dataDesc.returnedSamplesPerSeg << endl;
			outFile << dataDesc.returnedSegments << endl;
			// outFile << "# Voltage" << endl;

			for (ViInt32 i = 0; i < dataDesc.returnedSamplesPerSeg * dataDesc.returnedSegments; i++) {
				outFile << int(adcArrayP[i]) * dataDesc.vGain - dataDesc.vOffset << endl; // Volts
			}

			// Let's calculate mean and variance
			double mean = 0;
			double variance = 0;

			for (ViInt32 i = 0; i < dataDesc.returnedSamplesPerSeg * dataDesc.returnedSegments; i++) {
				mean += int(adcArrayP[i]) * dataDesc.vGain - dataDesc.vOffset;
			}

			mean = mean / (dataDesc.returnedSamplesPerSeg * dataDesc.returnedSegments);

			for (ViInt32 i = 0; i < dataDesc.returnedSamplesPerSeg * dataDesc.returnedSegments; i++) {
				variance += pow(int(adcArrayP[i]) * dataDesc.vGain - dataDesc.vOffset - mean, 2);
			}

			variance = variance / (dataDesc.returnedSamplesPerSeg * dataDesc.returnedSegments);

			cout << "Mean: " << mean << endl << "Variance: " << variance << endl;

			outFile.close();
			delete[] adcArrayP;

			cout << "Do this again? '1' for yes, '0' for no: ";
			cin >> workAgain;
		}
		else {
			cout << "Acquisition timeout..." << endl;
			cout << "Do this again? '1' for yes, '0' for no: ";
			cin >> workAgain;
		}*/
	}
	/* ============================================================================================================== */


	// TODO: make a special function for this shit
	// Stop the instruments
	Acqrs_closeAll(); // TODO: try to make for-cycle to stop each instrument independently
	cout << "All instruments successfully stopped." << endl;

	return 0;
}
