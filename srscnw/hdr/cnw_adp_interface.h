#ifndef SRSRAN_CNW_ADP_INTERFACES_H
#define SRSRAN_CNW_ADP_INTERFACES_H

#include <iostream>
/******************
 * CNW ADP Interfaces *
 ******************/
class cnw_interface_cnwadp // CNW -> ENB
{
public:
    virtual void run_cnw() = 0;
};

#endif