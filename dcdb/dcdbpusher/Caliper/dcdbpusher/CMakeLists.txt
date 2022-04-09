set(CALIPER_DCDBPUSHER_SOURCES
  DcdbPusher.cpp)

add_service_sources(${CALIPER_DCDBPUSHER_SOURCES})
add_caliper_service("dcdbpusher")
