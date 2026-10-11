#ifndef HELPER_FUNCTIONS_H
#define HELPER_FUNCTIONS_H
#include <QString>

double tx_duration(QString mode, double trPeriod, int nsps, bool bFast9);
// Decoded UTC fields omit fractional seconds (e.g. FT2 starts at :03.750).
bool received_in_second_period(unsigned utcSeconds, double trPeriod);

#endif // HELPER_FUNCTIONS_H
