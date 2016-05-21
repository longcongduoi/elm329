/**
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2009-2016 ObdDiag.Net. All rights reserved.
 *
 */

#include <lstring.h>
#include <algorithms.h>
#include <adaptertypes.h>

using namespace std;
using namespace util;

/**
 * Format the UID as string
 * @paramer[in] uid UID 3 x uint32_t array
 * @return UID as a string
 */
static string UIDToString(uint32_t uid[])
{
    string str(40);

    for (int j = 0; j < 4; j++) {
        NumericType value(uid[j]);

        for (int i = 3; i >= 0; i--) {
            str += to_ascii(value.bvalue[i] >> 4);
            str += to_ascii(value.bvalue[i] & 0x0F);
        }
        str += '-';
    }
    str.resize(str.length() - 1);
    return str;
}

/**
 * Display the LPC15XX CPU UID 
 */
void AdptReadSerialNum()
{
    uint32_t* uidBlock = reinterpret_cast<uint32_t*>(0x1FFFF7AC);
    AdptSendReply(UIDToString(uidBlock));
}

/**
 * Defines the low power mode
 */
void AdptPowerModeConfigure()
{
}
