/**
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2009-2016 ObdDiag.Net. All rights reserved.
 *
 */

#ifndef __PROTOCOL_ADAPTER_H__
#define __PROTOCOL_ADAPTER_H__

#include <adaptertypes.h>

// Command results
//
enum ReplyTypes {
    REPLY_OK =  1,
    REPLY_CMD_WRONG,
    REPLY_DATA_ERROR,
    REPLY_NO_DATA,
    REPLY_ERROR,
    REPLY_UNBL_2_CNNCT,
    REPLY_NONE,
    REPLY_BUS_BUSY,
    REPLY_BUS_ERROR,
    REPLY_CHKS_ERROR,
    REPLY_WIRING_ERROR
};

// Protocols
//
enum ProtocolTypes {
   PROT_AUTO = 0,
   PROT_ISO15765_1150 = 6,
   PROT_ISO15765_2950 = 7,
   PROT_ISO15765_1125,
   PROT_ISO15765_2925
};

// Adapters
//
enum AdapterTypes {
   ADPTR_AUTO,
   ADPTR_CAN,
   ADPTR_CAN_EXT
};

class ProtocolAdapter {
public:
    static ProtocolAdapter* getAdapter(int adapterType);
    virtual int onConnectEcu(bool sendReply) = 0;
    virtual int onRequest(const uint8_t* data, int len) = 0;
    virtual void getDescription() = 0;
    virtual void getDescriptionNum() = 0;
    virtual void dumpBuffer();
    virtual void setProtocol(int protocol) { connected_ = true; }
    virtual void closeProtocol() { connected_ = false; }
    virtual void open() { connected_ = false; }
    virtual void close() {}
    virtual void wiringCheck() = 0;
    virtual int getProtocol() const = 0;
    bool isConnected() const { return connected_; }
protected:
    ProtocolAdapter();
    bool           connected_;
    AdapterConfig* config_;
};

#endif //__PROTOCOL_ADAPTER_H__
