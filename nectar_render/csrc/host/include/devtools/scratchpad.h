#pragma once

#include <iostream>
#include <stdint.h>

#include "random/include/monte_carlo.h"

class ScratchPad {
public:

    __host__ ScratchPad() { }

    __host__ void run() {

        MonteCarlo mc;

        mc.estimate_pi();

    }

protected:



private:



};



