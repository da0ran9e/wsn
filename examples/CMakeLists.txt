build_lib_example(
  NAME wsn-sim
  SOURCE_FILES wsn-sim.cc
  LIBRARIES_TO_LINK
    ${libcore}
    ${libwsn}
)

build_lib_example(
  NAME bypass-example
  SOURCE_FILES bypass-example.cc
  LIBRARIES_TO_LINK
    ${libcore}
    ${libnetwork}
    ${libmobility}
    ${libspectrum}
    ${liblr-wpan}
    ${libwsn}
)

build_lib_example(
  NAME cc2420-example
  SOURCE_FILES cc2420-example.cc
  LIBRARIES_TO_LINK
    ${libcore}
    ${libnetwork}
    ${libmobility}
    ${libspectrum}
    ${libantenna}
    ${libpropagation}
    ${libwsn}
)
build_lib_example(
  NAME example1
  SOURCE_FILES example1.cc
  LIBRARIES_TO_LINK
    ${libcore}
    ${libnetwork}
    ${libmobility}
    ${libspectrum}
    ${libantenna}
    ${libpropagation}
    ${libwsn}
)

build_lib_example(
  NAME example2
  SOURCE_FILES 
    example2.cc
    scenarios/scenario1.cc
  LIBRARIES_TO_LINK
    ${libcore}
    ${libnetwork}
    ${libmobility}
    ${libspectrum}
    ${libantenna}
    ${libpropagation}
    ${libwsn}
)

build_lib_example(
  NAME example3
  SOURCE_FILES 
    example3.cc
    scenarios/scenario3.cc
  LIBRARIES_TO_LINK
    ${libcore}
    ${libnetwork}
    ${libmobility}
    ${libspectrum}
    ${libantenna}
    ${libpropagation}
    ${libwsn}
)

build_lib_example(
  NAME test_uav_range
  SOURCE_FILES 
    test_uav_range.cc
    scenarios/scenario3.cc
  LIBRARIES_TO_LINK
    ${libcore}
    ${libnetwork}
    ${libmobility}
    ${libspectrum}
    ${libantenna}
    ${libpropagation}
    ${libwsn}
)

build_lib_example(
  NAME example4
  SOURCE_FILES 
    example4.cc
    scenarios/scenario4/scenario4-api.cc
    scenarios/scenario4/scenario4-config.cc
    scenarios/scenario4/scenario4-scheduler.cc
    scenarios/scenario4/scenario4-metrics.cc
    scenarios/scenario4/scenario4-visualizer.cc
  LIBRARIES_TO_LINK
    ${libcore}
    ${libnetwork}
    ${libmobility}
    ${libspectrum}
    ${libantenna}
    ${libpropagation}
    ${libinternet}
    ${libwifi}
    ${libwsn}
)

build_lib_example(
  NAME example5
  SOURCE_FILES 
    example5.cc
    scenarios/scenario4/scenario4-api.cc
    scenarios/scenario4/scenario4-config.cc
    scenarios/scenario4/scenario4-scheduler.cc
    scenarios/scenario4/scenario4-metrics.cc
    scenarios/scenario4/scenario4-visualizer.cc
  LIBRARIES_TO_LINK
    ${libcore}
    ${libnetwork}
    ${libmobility}
    ${libspectrum}
    ${libantenna}
    ${libpropagation}
    ${libinternet}
    ${libwifi}
    ${libwsn}
)

#build_lib_example(
#  NAME uav-example
#  SOURCE_FILES uav-example.cc
#  LIBRARIES_TO_LINK
#    ${libcore}
#    ${libnetwork}
#    ${libmobility}
#    ${libspectrum}
#    ${libantenna}
#    ${libpropagation}
#    ${libwsn}
#)

#build_lib_example(
#  NAME cell-forming-example
#  SOURCE_FILES cell-forming-example.cc
#  LIBRARIES_TO_LINK
#    ${libcore}
#    ${libnetwork}
#    ${libmobility}
#    ${libspectrum}
#    ${libantenna}
#    ${libpropagation}
#    ${libwsn}
#)