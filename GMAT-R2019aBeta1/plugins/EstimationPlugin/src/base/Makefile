# $Id: Makefile 1343 2011-03-31 18:47:39Z  $
# Makefile for GMAT's OD Plugin
# This is the base level makefile 
#
# This file builds the plug-in base code

GMAT_Code_Location=../../../../src
GMAT_Binary_Location=../../../../../../gmat-dslwp-binary/GMAT/R2019aBeta1
C= gcc
CPP= g++
CFLAGS=-O2 -fPIC -g -Wall
CPPFLAGS=$(CFLAGS)

# handle different bin and lib directories here; for now, set to bin and lib for non-Mac platforms
ifeq ($(PLATFORM), mac)
else
GMAT_LIB_DIR = lib
GMAT_BIN_DIR = bin
GMAT_PLUGIN_DIR = plugins
endif

ifeq ($(LINUX_MAC), 1)

ifeq ($(PLATFORM), mac)
SHARED_EXTENSION = .dylib
SHARED_LIB_FLAGS = -dylib -dynamiclib -undefined dynamic_lookup \
                   -L$(GMAT_Code_Location)/base/$(GMAT_LIB_DIR) -l$(BASE_LIBRARY) \
                    $(MAC_CPP_FLAGS) $(ARCHITECTURE_FLAGS) $(LINK_FLAGS) -lxerces-c-3.1

# -reexport-lGmatBaseNoMatlab -dylib -dynamiclib -Wl -undefined dynamic_lookup

else
SHARED_EXTENSION = .so
SHARED_LIB_FLAGS = -shared $(LINK_FLAGS) -lxerces-c-3.1
# No longer in gcc 4.6:  --out-implib
endif

else

SHARED_EXTENSION = .dll
SHARED_LIB_FLAGS = -shared --out-implib $(LINK_FLAGS) -L$(GMAT_Code_Location)/base/$(GMAT_LIB_DIR) -lGmatBase

endif

SHARED_EXTENSION = .so
SHARED_LIB_FLAGS = -shared -lf2c -lGmatBase -lGmatUtil -L$(GMAT_Binary_Location)/bin

TARGET = ../../$(GMAT_LIB_DIR)/libGmatEstimation$(SHARED_EXTENSION)


ifeq ($(USE_DATAFILE_PLUGIN), 0)
DATAFILE_INCLUDES = -Imeasurementfile \
                    -ItdmReader

DATAFILE_OBJECTS = measurementfile/B3_obtype.o \
                   measurementfile/DataFile.o \
                   measurementfile/ObservationData.o \
                   measurementfile/ObType.o \
                   measurementfile/GmatObType.o \
                   measurementfile/GmatData.o \
                   measurementfile/RampTableData.o \
                   measurementfile/RampTableType.o \
                   tdmReader/TdmErrorHandler.o \
                   tdmReader/TdmObType.o \
                   tdmReader/TdmReadWriter.o \
                   factory/DataFileFactory.o \
                   factory/ObTypeFactory.o
else
DATAFILE_INCLUDES = -I$(GMAT_DataFile_Location)/base/datafile \
          -I$(GMAT_DataFile_Location)/base/datafile/CCSDS \
          -I$(GMAT_DataFile_Location)/base/datafile/CCSDS/CCSDSData \
          -I$(GMAT_DataFile_Location)/base/datafile/CCSDS/CCSDSMetaData \
          -I$(GMAT_DataFile_Location)/base/datafile/CCSDS/CCSDSObTypes

DATAFILE_OBJECTS = 
endif


ifeq ($(INCLUDE_IONOSPHERE), 1)
IONOSPHERE_C_OBJECTS = \
#	measurement/Ionosphere/cira.o \
#	measurement/Ionosphere/igrf.o \
#	measurement/Ionosphere/iridreg.o \
#	measurement/Ionosphere/irifun.o \
#	measurement/Ionosphere/iritec.o \
#	measurement/Ionosphere/irisub.o \
#	measurement/Ionosphere/loadfiles.o
	measurement/Ionosphere/iri2007.o
IONOSPHERE_CPP_OBJECTS = \
	measurement/Ionosphere/Ionosphere.o 
else
IONOSPHERE_C_OBJECTS = 
IONOSPHERE_CPP_OBJECTS = 
endif


all: prep $(TARGET)

prep:
	rm -rf $(TARGET)

HEADERS = -Iinclude \
          -Iadapter \
          -Icommand \
          -Idatafilter \
          -Ierrormodel \
          -Iestimator \
          -Ievent \
          -Ifactory \
          -Ihardware \
          -Iplugin \
          -Ipropagator \
          -Imeasurement \
	  -Imeasurementfile \
          -Imeasurement/Ionosphere \
          -Imeasurement/Troposphere \
          -Imeasurementmodel \
          -Ireporter \
          -Isignal \
          -Itrackingfile \
	  -ItdmReader \
          $(DATAFILE_INCLUDES) \
          -I$(GMAT_Code_Location)/base/include \
          -I$(GMAT_Code_Location)/base/foundation \
          -I$(GMAT_Code_Location)/base/event \
          -I$(GMAT_Code_Location)/base/executive \
          -I$(GMAT_Code_Location)/base/asset \
          -I$(GMAT_Code_Location)/base/attitude \
          -I$(GMAT_Code_Location)/base/hardware \
          -I$(GMAT_Code_Location)/base/spacecraft \
          -I$(GMAT_Code_Location)/base/asset \
          -I$(GMAT_Code_Location)/base/forcemodel \
          -I$(GMAT_Code_Location)/base/propagator \
          -I$(GMAT_Code_Location)/base/subscriber \
          -I$(GMAT_Code_Location)/base/command \
          -I$(GMAT_Code_Location)/base/solarsys \
          -I$(GMAT_Code_Location)/base/factory \
          -I$(GMAT_Code_Location)/base/stopcond \
          -I$(GMAT_Code_Location)/base/interpreter \
          -I$(GMAT_Code_Location)/base/util \
          -I$(GMAT_Code_Location)/base/interpolator \
          -I$(GMAT_Code_Location)/base/interface \
          -I$(GMAT_Code_Location)/base/parameter \
          -I$(GMAT_Code_Location)/base/configs \
          -I$(GMAT_Code_Location)/base/burn \
          -I$(GMAT_Code_Location)/base/solver \
          -I$(GMAT_Code_Location)/base/function \
          -I$(GMAT_Code_Location)/base/coordsystem \
          -I$(GMAT_Code_Location)/base/math \
          -I$(GMAT_Code_Location)/base/plugin \
          -I$(GMAT_Code_Location)/base/estimator \
	  -I$(GMAT_Code_Location)/gmatutil/include \
	  -I$(GMAT_Code_Location)/gmatutil/util \
	  -I$(GMAT_Code_Location)/base/factory/guicomponents \
	  -I$(GMAT_Code_Location)/gmatutil/util/datawriter \
	  -I$(GMAT_Code_Location)/base/forcemodel/harmonic \
	  -I$(GMAT_Code_Location)/gmatutil/util/matrixoperations \
          -I$(GMAT_Code_Location)/../depends/xerces/src \
          -I$(GMAT_Code_Location)/../depends/xerces/linux-install/include

clean : archclean

archclean :
	rm -rf */*.o
	rm -rf measurement/Ionosphere/*.o
	rm -rf measurement/Troposphere/*.o
	rm -rf $(TARGET)
	rm -rf */*~
	rm -rf core
	rm -rf */core

C_OBJECTS = \
        measurement/Ionosphere/iri2007.o

CPP_OBJECTS = \
        measurement/MeasurementModelBase.o \
        measurement/MediaCorrection.o \
        measurement/Ionosphere/Ionosphere.o \
        measurement/Troposphere/Troposphere.o \
        measurement/MeasurementData.o \
        measurement/MeasurementManager.o \
        trackingfile/TrackingFileSet.o \
        trackingfile/TFSMagicNumbers.o \
        factory/EstimationCommandFactory.o \
        factory/SignalModelFactory.o \
        factory/EstimatorHardwareFactory.o \
        factory/EstimationDataFilterFactory.o \
        factory/EventFactory.o \
        factory/MeasurementModelFactory.o \
        factory/ErrorModelFactory.o \
        factory/NavPropagatorFactory.o \
        factory/ObTypeFactory.o \
        factory/DataFileFactory.o \
        factory/EstimatorFactory.o \
        measurementmodel/MeasureModel.o \
        measurementmodel/GPSPointMeasureModel.o \
        datafilter/RejectFilter.o \
        datafilter/DataFilter.o \
        datafilter/AcceptFilter.o \
        event/LightTimeCorrection.o \
        event/EventData.o \
        event/EstimationRootFinder.o \
        event/EventManager.o \
        event/EventException.o \
        event/Event.o \
        tdmReader/TdmObType.o \
        tdmReader/TdmReadWriter.o \
        tdmReader/TdmErrorHandler.o \
        plugin/GmatPluginFunctions.o \
        reporter/ProgressReporter.o \
        estimator/BatchEstimator.o \
        estimator/Simulator.o \
        estimator/EstimationStateManager.o \
        estimator/Estimator.o \
        estimator/EstimatorException.o \
        estimator/BatchEstimatorBase.o \
        command/RunSimulator.o \
        command/RunEstimator.o \
        adapter/GPSAdapter.o \
        adapter/XSouthAdapter.o \
        adapter/DopplerAdapter.o \
        adapter/RightAscAdapter.o \
        adapter/YEastAdapter.o \
        adapter/PointRangeRateAdapterKps.o \
        adapter/GNDopplerAdapter.o \
        adapter/YNorthAdapter.o \
        adapter/TDRSRangeAdapter.o \
        adapter/DSNRangeAdapter.o \
        adapter/ElevationAdapter.o \
        adapter/GNRangeAdapter.o \
        adapter/XEastAdapter.o \
        adapter/DeclinationAdapter.o \
        adapter/TrackingDataAdapter.o \
        adapter/AzimuthAdapter.o \
        adapter/RangeAdapterKm.o \
        adapter/TDRSDopplerAdapter.o \
        adapter/RangeRateAdapterKps.o \
        adapter/AngleAdapterDeg.o \
        errormodel/ErrorModel.o \
        hardware/RFHardware.o \
        hardware/Receiver.o \
        hardware/Transmitter.o \
        hardware/Transponder.o \
        hardware/Signal.o \
        hardware/Sensor.o \
        hardware/Antenna.o \
        propagator/RungeKutta4.o \
        signal/SignalBase.o \
        signal/SignalData.o \
        signal/SinglePointSignal.o \
        signal/PhysicalSignal.o \
        measurementfile/RampTableType.o \
        measurementfile/DataFile.o \
        measurementfile/GmatObType.o \
        measurementfile/GmatData.o \
        measurementfile/RampTableData.o \
        measurementfile/ObservationData.o \
        measurementfile/B3_obtype.o \
        measurementfile/ObType.o \
        measurementfile/DataFileAdapter.o


ifeq ($(PLATFORM), mac)
$(TARGET) : $(C_OBJECTS) $(CPP_OBJECTS)
	mkdir -p ../../$(GMAT_LIB_DIR)
	$(CPP) $(C_OBJECTS) $(CPP_OBJECTS) -o $(TARGET) $(SHARED_LIB_FLAGS) $(PLATFORM_FLAGS)
	install_name_tool -change $(GMAT_LIB_DIR)/lib$(BASE_LIBRARY).dylib $(BASE_LIB_LOCATION)lib$(BASE_LIBRARY).dylib $(TARGET)
else
$(TARGET) : $(C_OBJECTS) $(CPP_OBJECTS)
	mkdir -p ../../$(GMAT_LIB_DIR)
	$(CPP) $(C_OBJECTS)  $(CPP_OBJECTS) -o $(TARGET) $(SHARED_LIB_FLAGS) $(PLATFORM_FLAGS)
endif


$(C_OBJECTS): %.o: %.c %.h
	$(C) $(CFLAGS) -c -o $@ $<

$(CPP_OBJECTS): %.o: %.cpp %.hpp
	$(CPP) $(CPPFLAGS) $(HEADERS) -c -o $@ $<
