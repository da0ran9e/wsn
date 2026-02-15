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
