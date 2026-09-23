set(COMMON_HEADERS MixDoctoratorIPC.h)

smtg_add_vst3plugin(MixDoctoratorSensor
    PACKAGE_NAME "125A-MixDoctorator-Sensor"
    SOURCES_LIST ${COMMON_HEADERS} SensorIDs.h SensorProcessor.h SensorProcessor.cpp
                 SensorController.h SensorController.cpp SensorFactory.cpp)
target_link_libraries(MixDoctoratorSensor PRIVATE sdk vstgui_support)
target_compile_features(MixDoctoratorSensor PRIVATE cxx_std_17)
if(WIN32)
    target_sources(MixDoctoratorSensor PRIVATE ${vst3sdk_SOURCE_DIR}/public.sdk/source/main/dllmain.cpp)
endif()
smtg_target_add_plugin_resources(MixDoctoratorSensor RESOURCES ../resource/Sensor.uidesc)
smtg_target_configure_version_file(MixDoctoratorSensor)

smtg_add_vst3plugin(MixDoctoratorBrain
    PACKAGE_NAME "125A-MixDoctorator-Brain"
    SOURCES_LIST ${COMMON_HEADERS} BrainIDs.h BrainProcessor.h BrainProcessor.cpp
                 BrainController.h BrainController.cpp BrainFactory.cpp)
target_link_libraries(MixDoctoratorBrain PRIVATE sdk vstgui_support)
target_compile_features(MixDoctoratorBrain PRIVATE cxx_std_17)
if(WIN32)
    target_sources(MixDoctoratorBrain PRIVATE ${vst3sdk_SOURCE_DIR}/public.sdk/source/main/dllmain.cpp)
endif()
smtg_target_add_plugin_resources(MixDoctoratorBrain RESOURCES ../resource/Brain.uidesc)
smtg_target_configure_version_file(MixDoctoratorBrain)
