#include "helper_functions.h"
#include "models/Modes.hpp"
#include "models/FrequencyList.hpp"
#include "models/Bands.hpp"
#include "Logger.hpp"
#include <QCoreApplication>
#include <iostream>
#include <stdexcept>

BOOST_LOG_GLOBAL_LOGGER_CTOR_ARGS(sys,
  boost::log::sources::severity_channel_logger_mt<boost::log::trivial::severity_level>,
  (boost::log::keywords::channel = "FT2 test"));

void check(bool ok, char const *message)
{
  if(!ok) throw std::runtime_error(message);
}

int main(int argc, char **argv)
{
  QCoreApplication app {argc, argv};
  try {
    check(Modes::FT4==9 && Modes::Q65==12, "Existing persisted mode IDs changed");
    check(Modes::value("FT2")==Modes::FT2, "FT2 mode lookup");
    check(QString {Modes::name(Modes::FT2)}=="FT2", "FT2 mode name");
    unsigned const starts[] {0,3,7,11,15,18,22,26,30,33,37,41,45,48,52,56};
    for(unsigned minute : {0u, 60u, 86340u}) {
      for(unsigned i=0; i<16; ++i) {
        check(received_in_second_period(minute+starts[i],3.75)==bool(i%2), "FT2 reply sequence");
      }
    }
    check(!received_in_second_period(0,0), "Invalid period");
    check(tx_duration("FT2",3.75,3456,false)>2.52 && tx_duration("FT2",3.75,3456,false)<3.75, "FT2 transmit window");
    Bands bands;
    FrequencyList_v2_101 frequencies {&bands};
    FrequencyList_v2_101::Item custom {14074500, Modes::FT8, IARURegions::ALL, "user entry", "", {}, {}, true};
    frequencies.frequency_list({custom});
    frequencies.add_mode_defaults(Modes::FT2);
    auto items=frequencies.frequency_list();
    check(items.contains(custom), "Existing frequency lost");
    bool has20m=false;
    for(auto const& item : items) has20m |= item.mode_==Modes::FT2 && item.frequency_==14084000;
    check(has20m, "FT2 defaults missing");
    frequencies.add_mode_defaults(Modes::FT2);
    check(items==frequencies.frequency_list(), "FT2 defaults duplicated");
    custom.mode_=Modes::FT2;
    frequencies.frequency_list({custom});
    frequencies.add_mode_defaults(Modes::FT2);
    check(frequencies.frequency_list()==FrequencyList_v2_101::FrequencyItems {custom}, "User FT2 frequencies replaced");
    std::cout << "FT2 mode, timing and frequency migration checks passed.\n";
  } catch(std::exception const& error) {
    std::cerr << error.what() << '\n';
    return 1;
  }
}
