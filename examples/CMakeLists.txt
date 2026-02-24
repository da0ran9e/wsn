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